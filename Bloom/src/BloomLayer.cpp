#include "BloomLayer.h"

#include "Wire/Core/Application.h"
#include "Wire/Renderer/Device.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

namespace bloom {

    namespace utils {

        inline static uint32_t getBloomMipCount(uint32_t width, uint32_t height, uint32_t maxMips = 8, uint32_t minDimension = 10)
		{
			uint32_t mips = 0;
			while (mips < maxMips && std::min(width, height) > minDimension)
			{
				width /= 2;
				height /= 2;
				mips++;
			}

			return mips;
		}

    }

    struct BloomVertex0
    {
        glm::vec4 Position;
        glm::vec4 Color;
    };

    struct BrightPassPushConstants
    {
        glm::ivec2 SourceSize;
        glm::ivec2 DestinationSize;
        float Threshold;
        float Intensity;
    };

    struct BlurPushConstants
    {
        glm::ivec2 FullSize;
        int Horizontal;
    };

    struct CombinePushConstants
    {
        float BloomStrength;
    };

    void BloomLayer::onAttach()
    {
        m_Device = wire::Application::get().getDevice();

        m_MipCount = utils::getBloomMipCount((uint32_t)m_Device->getExtent().x, (uint32_t)m_Device->getExtent().y);

        wire::AttachmentDesc colorAttachment;
        colorAttachment.Format = wire::AttachmentFormat::RGBA16_SFloat;
        colorAttachment.Usage = wire::AttachmentLayout::Color;
        colorAttachment.PreviousAttachmentUsage = wire::AttachmentLayout::Undefined;
        colorAttachment.Samples = wire::AttachmentDesc::Count1Bit;
        colorAttachment.LoadOp = wire::LoadOperation::Clear;
        colorAttachment.StoreOp = wire::StoreOperation::Store;
        colorAttachment.StencilLoadOp = wire::LoadOperation::DontCare;
        colorAttachment.StencilStoreOp = wire::StoreOperation::DontCare;
        colorAttachment.BlendState = {};

        wire::AttachmentDesc depthAttachment;
        depthAttachment.Format = wire::AttachmentFormat::D32_SFloat;
        depthAttachment.Usage = wire::AttachmentLayout::Depth;
        depthAttachment.PreviousAttachmentUsage = wire::AttachmentLayout::Undefined;
        depthAttachment.Samples = wire::AttachmentDesc::Count1Bit;
        depthAttachment.LoadOp = wire::LoadOperation::Clear;
        depthAttachment.StoreOp = wire::StoreOperation::Store;
        depthAttachment.StencilLoadOp = wire::LoadOperation::DontCare;
        depthAttachment.StencilStoreOp = wire::StoreOperation::DontCare;
        depthAttachment.BlendState = wire::BlendState{ .BlendEnable = false };

        wire::FramebufferDesc framebufferInfo{};
        framebufferInfo.Format = colorAttachment.Format;
        framebufferInfo.Usage = wire::AttachmentUsage::ColorAttachment | wire::AttachmentUsage::Storage | wire::AttachmentUsage::Sampled;
        framebufferInfo.Extent = m_Device->getExtent();
        framebufferInfo.Layout = wire::AttachmentLayout::Undefined;
        framebufferInfo.MipCount = 1;
		framebufferInfo.HasDepth = true;
		framebufferInfo.DepthFormat = depthAttachment.Format;

        m_ColorFramebuffer = m_Device->createFramebuffer(framebufferInfo, "BloomLayer::m_ColorFramebuffer");

        wire::RenderPassDesc renderPassInfo{};
        renderPassInfo.Attachments = {
            colorAttachment, depthAttachment
        };

        m_ColorRenderPass = m_Device->createRenderPass(renderPassInfo, m_ColorFramebuffer, "BloomLayer::m_RenderPass");

        wire::InputLayout layout{};
        layout.VertexBufferLayout = {
            { "POSITION", wire::ShaderDataType::Float4, sizeof(glm::vec4), offsetof(BloomVertex0, Position) },
            { "COLOR", wire::ShaderDataType::Float4, sizeof(glm::vec4), offsetof(BloomVertex0, Color) },
        };
        layout.Stride = sizeof(BloomVertex0);
        layout.ResourceLayout = nullptr;
        layout.PushConstantInfos = {
            { sizeof(glm::mat4), 0, wire::ShaderType::Vertex }
        };

        std::array vertices = {
            BloomVertex0{ { -0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 2.7f, 2.0f, 1.0f } },
            BloomVertex0{ {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 2.7f, 2.0f, 1.0f } },
            BloomVertex0{ {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 2.7f, 2.0f, 1.0f } },
            BloomVertex0{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 2.7f, 2.0f, 1.0f } },
        };

        m_ColorVertexBuffer = m_Device->createBuffer(wire::VertexBuffer, sizeof(BloomVertex0) * vertices.size(), vertices.data());

        std::array<uint32_t, 6> indices = {
            0, 1, 2, 2, 3, 0
        };

        m_IndexBuffer = m_Device->createBuffer(wire::IndexBuffer, sizeof(uint32_t) * indices.size(), indices.data());

        wire::GraphicsPipelineDesc pipelineInfo{};
        pipelineInfo.Layout = layout;
        pipelineInfo.RenderPass = m_ColorRenderPass;
        pipelineInfo.ShaderPath = "shadercache://BloomColor.hlsl";
        pipelineInfo.Topology = wire::PrimitiveTopology::TriangleList;

        m_ColorPipeline = m_Device->createGraphicsPipeline(pipelineInfo, "BloomLayer::m_GraphicsPipeline");

        wire::ShaderResourceLayoutInfo computeResources{};
        computeResources.Sets = {
            wire::ShaderResourceSetInfo{
                .Resources = {
                    wire::ShaderResourceInfo{
                        .Binding = 0,
                        .Type = wire::ShaderResourceType::SampledImage,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    },
                    wire::ShaderResourceInfo{
                        .Binding = 1,
                        .Type = wire::ShaderResourceType::Sampler,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    },
                    wire::ShaderResourceInfo{
                        .Binding = 2,
                        .Type = wire::ShaderResourceType::StorageImage,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    }
                }
            }
        };
        m_BrightPassResourceLayout = m_Device->createShaderResourceLayout(computeResources, "BloomLayer::m_BrightPassResourceLayout");
        
        for (size_t i = 0; i < m_MipCount; i++)
        {
            std::string debugName = "BloomLayer::m_BrightPassResources[" + std::to_string(i) + "]";
            m_BrightPassResources.push_back(m_Device->createShaderResource(0, m_BrightPassResourceLayout, debugName));
        }

        framebufferInfo.MipCount = m_MipCount;
        framebufferInfo.HasDepth = false;
        framebufferInfo.Usage = wire::AttachmentUsage::Storage | wire::AttachmentUsage::Sampled;
        framebufferInfo.Layout = wire::AttachmentLayout::General;
        m_BrightPassFramebuffer = m_Device->createFramebuffer(framebufferInfo, "BloomLayer::m_BrightPassFramebuffer");

        wire::ComputeInputLayout computeLayout{};
        computeLayout.ResourceLayout = m_BrightPassResourceLayout;
        computeLayout.PushConstantInfos = {
            { sizeof(BrightPassPushConstants), 0, wire::ShaderType::Compute }
        };

        wire::ComputePipelineDesc computeInfo{};
        computeInfo.Layout = computeLayout;
        computeInfo.ShaderPath = "shadercache://BloomBrightPassDownsample.compute.hlsl";

        m_BrightPassDownsamplePipeline = m_Device->createComputePipeline(computeInfo, "BloomLayer::m_BrightPassDownsamplePipeline");

        wire::SamplerDesc samplerInfo{};
        samplerInfo.MinFilter = wire::SamplerFilter::Linear;
        samplerInfo.MagFilter = wire::SamplerFilter::Linear;
        samplerInfo.AddressModeU = wire::AddressMode::ClampToEdge;
        samplerInfo.AddressModeV = wire::AddressMode::ClampToEdge;
        samplerInfo.AddressModeW = wire::AddressMode::ClampToEdge;
        samplerInfo.EnableAnisotropy = false;
        samplerInfo.BorderColor = wire::BorderColor::FloatOpaqueBlack;
        samplerInfo.MipmapMode = wire::MipmapMode::Linear;
        samplerInfo.CompareEnable = false;
        samplerInfo.MinLod = 0.0f;
        samplerInfo.NoMaxLodClamp = true;
        samplerInfo.MipLodBias = 0.0f;

        m_BrightPassSampler = m_Device->createSampler(samplerInfo, "BloomLayer::m_BrightPassSampler");
        
        std::shared_ptr<wire::Texture2D> colorTexture = m_ColorFramebuffer->asTexture2D();
        std::shared_ptr<wire::Texture2D> brightPassTexture = m_BrightPassFramebuffer->asTexture2D();

        m_BrightPassResources[0]->update(colorTexture, 0, 0);
        m_BrightPassResources[0]->update(m_BrightPassSampler, 1, 0);
        m_BrightPassResources[0]->update(m_BrightPassFramebuffer, 2, 0, 1);

        for (uint32_t i = 1; i < m_MipCount - 1; i++)
        {
            m_BrightPassResources[i]->update(brightPassTexture, 0, 0, i);
            m_BrightPassResources[i]->update(m_BrightPassSampler, 1, 0);
            m_BrightPassResources[i]->update(m_BrightPassFramebuffer, 2, 0, i + 1);
        }

        m_BlurResourceLayout = m_Device->createShaderResourceLayout(computeResources, "BloomLayer::m_BlurResourceLayout");
        m_BlurFramebuffer = m_Device->createFramebuffer(framebufferInfo, "BloomLayer::m_BlurFramebuffer");
        m_BlurIntermediateFramebuffer = m_Device->createFramebuffer(framebufferInfo, "BloomLayer::m_BlurIntermediateFramebuffer");

        computeLayout.PushConstantInfos = {
            { sizeof(BlurPushConstants), 0, wire::ShaderType::Compute }
        };
        computeInfo.Layout = computeLayout;
        computeInfo.ShaderPath = "shadercache://BloomBlur.compute.hlsl";
        m_BlurPipeline = m_Device->createComputePipeline(computeInfo, "BloomLayer::m_BlurPipeline");

        for (size_t i = 0; i < m_MipCount - 2; i++)
        {
            std::string debugName = "BloomLayer::m_BlurResources[" + std::to_string(i) + "]";
            auto& resource = m_BlurResources.emplace_back();
            std::string debugName0 = debugName + "[0]";
            std::string debugName1 = debugName + "[1]";
            resource[0] = m_Device->createShaderResource(0, m_BlurResourceLayout, debugName0);
            resource[1] = m_Device->createShaderResource(0, m_BlurResourceLayout, debugName1);
        }

        std::shared_ptr<wire::Texture2D> intermediateTexture = m_BlurIntermediateFramebuffer->asTexture2D();

        for (uint32_t i = 0; i < m_MipCount - 2; i++)
        {
            m_BlurResources[i][0]->update(brightPassTexture, 0, 0, m_MipCount - (i + 1));
            m_BlurResources[i][0]->update(m_BrightPassSampler, 1, 0);
            m_BlurResources[i][0]->update(m_BlurIntermediateFramebuffer, 2, 0, m_MipCount - (i + 1));

            m_BlurResources[i][1]->update(intermediateTexture, 0, 0, m_MipCount - (i + 1));
            m_BlurResources[i][1]->update(m_BrightPassSampler, 1, 0);
            m_BlurResources[i][1]->update(m_BlurFramebuffer, 2, 0, m_MipCount - (i + 1));
        }

        computeResources = wire::ShaderResourceLayoutInfo{
            .Sets = {
                wire::ShaderResourceSetInfo{
                    .Resources = {
                        wire::ShaderResourceInfo{
                            .Binding = 0,
                            .Type = wire::ShaderResourceType::SampledImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 1,
                            .Type = wire::ShaderResourceType::Sampler,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 2,
                            .Type = wire::ShaderResourceType::SampledImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 3,
                            .Type = wire::ShaderResourceType::Sampler,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 4,
                            .Type = wire::ShaderResourceType::StorageImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                    }
                }
            }
        };
        m_UpsampleResourceLayout = m_Device->createShaderResourceLayout(computeResources, "BloomLayer::m_UpsampleResourceLayout");

        for (size_t i = 0; i < m_MipCount - 1; i++)
        {
            std::string debugName = "BloomLayer::m_UpsampleResources[" + std::to_string(i) + "]";
            m_UpsampleResources.push_back(m_Device->createShaderResource(0, m_UpsampleResourceLayout, debugName));
        }

        computeLayout.ResourceLayout = m_UpsampleResourceLayout;

        computeInfo.Layout = computeLayout;
        computeInfo.ShaderPath = "shadercache://BloomUpsample.compute.hlsl";

        m_UpsamplePipeline = m_Device->createComputePipeline(computeInfo, "BloomLayer::m_UpsamplePipeline");

        wire::SamplerDesc lowSamplerInfo{};
        lowSamplerInfo.MagFilter = wire::SamplerFilter::Linear;
        lowSamplerInfo.MinFilter = wire::SamplerFilter::Linear;
        lowSamplerInfo.MipmapMode = wire::MipmapMode::Linear;
        lowSamplerInfo.AddressModeU = wire::AddressMode::ClampToEdge;
        lowSamplerInfo.AddressModeV = wire::AddressMode::ClampToEdge;
        lowSamplerInfo.AddressModeW = wire::AddressMode::ClampToEdge;
        lowSamplerInfo.MipLodBias = 0.0f;
        lowSamplerInfo.EnableAnisotropy = false;
        lowSamplerInfo.MaxAnisotropy = 1.0f;
        lowSamplerInfo.CompareEnable = false;
        lowSamplerInfo.MinLod = 0.0f;
        lowSamplerInfo.MaxLod = 1000.0f;
        lowSamplerInfo.BorderColor = wire::BorderColor::IntOpaqueBlack;

        m_UpsampleLowSampler = m_Device->createSampler(lowSamplerInfo, "BloomLayer::m_UpsampleLowSampler");

        wire::SamplerDesc highSamplerInfo{};
        highSamplerInfo.MagFilter = wire::SamplerFilter::Linear;
        highSamplerInfo.MinFilter = wire::SamplerFilter::Linear;
        highSamplerInfo.MipmapMode = wire::MipmapMode::Linear;
        highSamplerInfo.AddressModeU = wire::AddressMode::ClampToEdge;
        highSamplerInfo.AddressModeV = wire::AddressMode::ClampToEdge;
        highSamplerInfo.AddressModeW = wire::AddressMode::ClampToEdge;
        highSamplerInfo.MipLodBias = 0.0f;
        highSamplerInfo.EnableAnisotropy = false;
        highSamplerInfo.MaxAnisotropy = 1.0f;
        highSamplerInfo.CompareEnable = false;
        highSamplerInfo.MinLod = 0.0f;
        highSamplerInfo.MaxLod = 1000.0f;
        highSamplerInfo.BorderColor = wire::BorderColor::IntOpaqueBlack;
        
        m_UpsampleHighSampler = m_Device->createSampler(highSamplerInfo, "BloomLayer::m_UpsampleHighSampler");
        m_UpsampleFramebuffer = m_Device->createFramebuffer(framebufferInfo, "BloomLayer::m_UpsampleFramebuffer");

        std::shared_ptr<wire::Texture2D> blurTexture = m_BlurFramebuffer->asTexture2D();
        std::shared_ptr<wire::Texture2D> upsampleTexture = m_UpsampleFramebuffer->asTexture2D();

        m_UpsampleResources[0]->update(blurTexture, 0, 0, m_MipCount - 1);
        m_UpsampleResources[0]->update(m_UpsampleLowSampler, 1, 0);
        m_UpsampleResources[0]->update(blurTexture, 2, 0, m_MipCount - 2);
        m_UpsampleResources[0]->update(m_UpsampleHighSampler, 3, 0);
        m_UpsampleResources[0]->update(m_UpsampleFramebuffer, 4, 0, m_MipCount - 2);

        for (uint32_t i = 1; i < m_MipCount - 3; i++)
        {
            m_UpsampleResources[i]->update(upsampleTexture, 0, 0, m_MipCount - (i + 1));
            m_UpsampleResources[i]->update(m_UpsampleLowSampler, 1, 0);
            m_UpsampleResources[i]->update(blurTexture, 2, 0, m_MipCount - (i + 2));
            m_UpsampleResources[i]->update(m_UpsampleHighSampler, 3, 0);
            m_UpsampleResources[i]->update(m_UpsampleFramebuffer, 4, 0, m_MipCount - (i + 2));
        }

        // for (uint32_t i = 0; i < m_MipCount - 3; i++) // first two mips ignored as they were not blurred
        // {
        //     m_UpsampleResources[i]->update(blurTexture, 0, 0, m_BlurFramebuffer->getNumMips() - (i + 1));
        //     m_UpsampleResources[i]->update(m_UpsampleLowSampler, 1, 0);
        //     m_UpsampleResources[i]->update(blurTexture, 2, 0, m_BlurFramebuffer->getNumMips() - (i + 2));
        //     m_UpsampleResources[i]->update(m_UpsampleHighSampler, 3, 0);
        //     m_UpsampleResources[i]->update(m_UpsampleFramebuffer, 4, 0, m_MipCount - (i + 2));
        // }

        // deal with last two mips
        m_UpsampleResources[m_MipCount - 3]->update(upsampleTexture, 0, 0, 2);
        m_UpsampleResources[m_MipCount - 3]->update(m_UpsampleLowSampler, 1, 0);
        m_UpsampleResources[m_MipCount - 3]->update(brightPassTexture, 2, 0, 1);
        m_UpsampleResources[m_MipCount - 3]->update(m_UpsampleHighSampler, 3, 0);
        m_UpsampleResources[m_MipCount - 3]->update(m_UpsampleFramebuffer, 4, 0, 1);

        m_UpsampleResources[m_MipCount - 2]->update(upsampleTexture, 0, 0, 1);
        m_UpsampleResources[m_MipCount - 2]->update(m_UpsampleLowSampler, 1, 0);
        m_UpsampleResources[m_MipCount - 2]->update(brightPassTexture, 2, 0, 0);
        m_UpsampleResources[m_MipCount - 2]->update(m_UpsampleHighSampler, 3, 0);
        m_UpsampleResources[m_MipCount - 2]->update(m_UpsampleFramebuffer, 4, 0, 0);

        wire::RenderPassDesc combineRenderPassInfo{};
        combineRenderPassInfo.Attachments = {
            wire::AttachmentDesc{
                .Format = wire::AttachmentFormat::SwapchainColorDefault,
                .Usage = wire::AttachmentLayout::Color,
                .PreviousAttachmentUsage = wire::AttachmentLayout::Undefined,
                .Samples = wire::AttachmentDesc::Count1Bit,
                .LoadOp = wire::LoadOperation::Clear,
                .StoreOp = wire::StoreOperation::Store,
                .StencilLoadOp = wire::LoadOperation::DontCare,
                .StencilStoreOp = wire::StoreOperation::DontCare,
                .BlendState = wire::BlendState{
                    .BlendEnable = true,
                    .SrcColorBlendFactor = wire::BlendFactor::One,
                    .DstColorBlendFactor = wire::BlendFactor::One,
                    .ColorBlendOp = wire::BlendOperation::Add,
                    .SrcAlphaBlendFactor = wire::BlendFactor::One,
                    .DstAlphaBlendFactor = wire::BlendFactor::One,
                    .AlphaBlendOp = wire::BlendOperation::Add,
                }
            }
        };

        m_CombineRenderPass = m_Device->createRenderPass(combineRenderPassInfo, m_Device->getSwapchain(), "BloomLayer::m_CombineRenderPassInfo");

        wire::ShaderResourceLayoutInfo pixelResources = wire::ShaderResourceLayoutInfo{
            .Sets = {
                wire::ShaderResourceSetInfo{
                    .Resources = {
                        wire::ShaderResourceInfo{
                            .Binding = 0,
                            .Type = wire::ShaderResourceType::SampledImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Pixel
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 1,
                            .Type = wire::ShaderResourceType::SampledImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Pixel
                        },
                        wire::ShaderResourceInfo{
                            .Binding = 2,
                            .Type = wire::ShaderResourceType::Sampler,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Pixel
                        }
                    }
                }
            }
        };
        m_CombineResourceLayout = m_Device->createShaderResourceLayout(pixelResources, "BloomLayer::m_CombineResourceLayout");
        m_CombineResource = m_Device->createShaderResource(0, m_CombineResourceLayout, "BloomLayer::m_CombineResource");

        wire::InputLayout combineLayout{};
        combineLayout.ResourceLayout = m_CombineResourceLayout;
        combineLayout.PushConstantInfos = {
            { sizeof(CombinePushConstants), 0, wire::ShaderType::Pixel }
        };
        combineLayout.VertexBufferLayout = {};
        combineLayout.Stride = 0;
        
        wire::GraphicsPipelineDesc combineInfo{};
        combineInfo.Layout = combineLayout;
        combineInfo.ShaderPath = "shadercache://BloomCombine.hlsl";
        combineInfo.RenderPass = m_CombineRenderPass;
        combineInfo.Topology = wire::PrimitiveTopology::TriangleList;
    
        m_CombinePipeline = m_Device->createGraphicsPipeline(combineInfo, "BloomLayer::m_CombinePipeline");

        wire::SamplerDesc combineSamplerInfo{};
        combineSamplerInfo.MinFilter = wire::SamplerFilter::Linear;
        combineSamplerInfo.MagFilter = wire::SamplerFilter::Linear;
        combineSamplerInfo.AddressModeU = wire::AddressMode::ClampToEdge;
        combineSamplerInfo.AddressModeV = wire::AddressMode::ClampToEdge;
        combineSamplerInfo.AddressModeW = wire::AddressMode::ClampToEdge;
        combineSamplerInfo.MipmapMode = wire::MipmapMode::Linear;
        combineSamplerInfo.MinLod = 0.0f;
        combineSamplerInfo.MaxLod = 0.0f;

        m_CombineSampler = m_Device->createSampler(combineSamplerInfo, "BloomLayer::m_CombineSampler");

        m_CombineResource->update(colorTexture, 0, 0);
        m_CombineResource->update(upsampleTexture, 1, 0, 0);
        m_CombineResource->update(m_CombineSampler, 2, 0);

        m_Device->drop(upsampleTexture);
        m_Device->drop(blurTexture);
        m_Device->drop(intermediateTexture);
        m_Device->drop(brightPassTexture);
        m_Device->drop(colorTexture);

        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            m_CommandLists[i] = m_Device->createCommandList();
    }

    void BloomLayer::onDetach()
    {
    }

    void BloomLayer::onImGuiRender()
    {
        ImGui::Begin("Bloom settings");

        ImGui::SliderFloat("Threshold", &m_Threshold, 0.0f, 4.0f, "%.1f");
        ImGui::SliderFloat("Intensity", &m_Intensity, 0.0f, 5.0f, "%.1f");
        ImGui::SliderFloat("Bloom Strength", &m_BloomStrength, 0.0f, 1.0f, "%.2f");

        ImGui::End();
    }

    void BloomLayer::onUpdate(float timestep)
    {
        wire::CommandList& commandList = m_CommandLists[m_Device->getFrameIndex()];

        glm::vec2 extent = m_Device->getExtent();
        float aspect = extent.x / extent.y;

        commandList.begin();
        commandList.beginRenderPass(m_ColorRenderPass);
        commandList.bindPipeline(m_ColorPipeline);
        commandList.setScissor({ 0, 0 }, extent);
        commandList.setViewport({ 0, 0 }, extent, 0.0f, 1.0f);
        commandList.pushConstants(wire::ShaderType::Vertex, glm::ortho(-aspect, aspect, -1.0f, 1.0f));
        commandList.bindIndexBuffer(m_IndexBuffer);
        commandList.bindVertexBuffers({ m_ColorVertexBuffer });

        commandList.drawIndexed(6);

        commandList.endRenderPass();

        commandList.imageMemoryBarrier(m_ColorFramebuffer, wire::AttachmentLayout::Color, wire::AttachmentLayout::ShaderReadOnly);

        BrightPassPushConstants brightPassPushConstants{
            .SourceSize = (glm::ivec2)extent,
            .DestinationSize = (glm::ivec2)extent / 2,
            .Threshold = m_Threshold,
            .Intensity = m_Intensity
        };

        std::vector<glm::ivec2> sizes;

        commandList.bindPipeline(m_BrightPassDownsamplePipeline);

        sizes.push_back(brightPassPushConstants.SourceSize);
        for (uint32_t i = 0; i < m_MipCount - 1; i++)
        {
            commandList.pushConstants(wire::ShaderType::Compute, brightPassPushConstants);
            commandList.bindShaderResource(0, m_BrightPassResources[i]);

            uint32_t groupCountX = ((uint32_t)extent.x + 15) / 16;
            uint32_t groupCountY = ((uint32_t)extent.y + 15) / 16;
            commandList.dispatch(groupCountX, groupCountY, 1);

            commandList.imageMemoryBarrier(m_BrightPassFramebuffer, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, i + 1);

            brightPassPushConstants.SourceSize = brightPassPushConstants.DestinationSize;
            brightPassPushConstants.DestinationSize /= 2;

            sizes.push_back(brightPassPushConstants.SourceSize);
        }

        BlurPushConstants blurPushConstants{};

        commandList.bindPipeline(m_BlurPipeline);

        uint32_t sizeIndex = static_cast<uint32_t>(sizes.size()) - 1;
        for (uint32_t i = 0; i < m_BlurResources.size(); i++)
        {
            blurPushConstants.FullSize = sizes[sizeIndex--];
            blurPushConstants.Horizontal = 1;

            commandList.pushConstants(wire::ShaderType::Compute, blurPushConstants);
            commandList.bindShaderResource(0, m_BlurResources[i][0]);

            uint32_t groupCountX = ((uint32_t)blurPushConstants.FullSize.x + 15) / 16;
            uint32_t groupCountY = ((uint32_t)blurPushConstants.FullSize.y + 15) / 16;
            commandList.dispatch(groupCountX, groupCountY, 1);

            blurPushConstants.Horizontal = 0;
            commandList.imageMemoryBarrier(m_BlurIntermediateFramebuffer, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, m_MipCount - (i + 1));

            commandList.pushConstants(wire::ShaderType::Compute, blurPushConstants);
            commandList.bindShaderResource(0, m_BlurResources[i][1]);

            commandList.dispatch(groupCountX, groupCountY, 1);
        }

        sizeIndex = static_cast<uint32_t>(sizes.size()) - 2;

        commandList.bindPipeline(m_UpsamplePipeline);

        commandList.imageMemoryBarrier(m_BrightPassFramebuffer, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, 0, 1);
        commandList.imageMemoryBarrier(m_BlurFramebuffer, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, 0, m_BlurFramebuffer->getNumMips());

        for (uint32_t i = 0; i < m_UpsampleResources.size(); i++)
        {
            commandList.bindShaderResource(0, m_UpsampleResources[i]);

            uint32_t groupCountX = ((uint32_t)extent.x + 15) / 16;
            uint32_t groupCountY = ((uint32_t)extent.y + 15) / 16;
            commandList.dispatch(groupCountX, groupCountY, 1);

            commandList.imageMemoryBarrier(m_UpsampleFramebuffer, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, m_MipCount - (i + 2));
        }

        CombinePushConstants combinePushConstants{
            .BloomStrength = m_BloomStrength
        };

        commandList.beginRenderPass(m_CombineRenderPass);
        commandList.bindPipeline(m_CombinePipeline);
        commandList.setScissor({ 0, 0 }, extent);
        commandList.setViewport({ 0, 0 }, extent, 0.0f, 1.0f);
        commandList.pushConstants(wire::ShaderType::Pixel, combinePushConstants);
        commandList.bindShaderResource(0, m_CombineResource);

        commandList.draw(3);

        commandList.endRenderPass();

        commandList.imageMemoryBarrier(m_BlurFramebuffer, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General, 0, m_BlurFramebuffer->getNumMips());
        commandList.imageMemoryBarrier(m_BrightPassFramebuffer, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General, 0, m_MipCount);
        commandList.imageMemoryBarrier(m_BlurIntermediateFramebuffer, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General, 2, m_MipCount - 2);
        commandList.imageMemoryBarrier(m_UpsampleFramebuffer, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General, 0, m_MipCount - 1);

        commandList.end();

        m_Device->submitCommandList(commandList);
    }

}
