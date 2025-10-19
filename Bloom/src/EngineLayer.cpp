#include "EngineLayer.h"

#include "tiny_obj_loader.h"
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <unordered_map>

namespace bloom {

	namespace Utils {

		static void LoadModel(const std::filesystem::path& path, std::vector<ModelVertex>& modelVertices, std::vector<uint32_t>& modelIndices)
		{
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warning, error;

			std::unordered_map<ModelVertex, uint32_t> uniqueVertices;

			std::string pathStr = path.string();
			bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, pathStr.c_str());

			if (!warning.empty())
				WR_WARN("TinyObj Warning: {}", warning);
			if (!error.empty())
				WR_ERROR("TinyObj Error: {}", error);

			WR_ASSERT(success, "Failed to load object with tinyobj!");

			for (const auto& shape : shapes)
			{
				for (const auto& index : shape.mesh.indices)
				{
					ModelVertex vertex{};

					vertex.Position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
						1.0f
					};

					if (index.texcoord_index >= 0)
					{
						vertex.TexCoord = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
					}
					else
						vertex.TexCoord = { 0.0f, 0.0f };

					vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

					if (index.normal_index >= 0)
					{
						vertex.Normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};
					}

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(modelVertices.size());
						modelVertices.push_back(vertex);
					}

					modelIndices.push_back(uniqueVertices[vertex]);
				}
			}
		}

	}

	void EngineLayer::onAttach()
	{
		m_Renderer = wire::Application::get().getRenderer();
        
        wire::RenderPassDesc renderPassDesc{};
        wire::AttachmentDesc colorAttachment{};
        colorAttachment.Format = wire::AttachmentFormat::SwapchainColorDefault;
        colorAttachment.Usage = wire::AttachmentLayout::Present;
        colorAttachment.PreviousAttachmentUsage = wire::AttachmentLayout::Undefined;
        colorAttachment.Samples = wire::AttachmentDesc::Count1Bit;
        colorAttachment.LoadOp = wire::LoadOperation::Clear;
        colorAttachment.StoreOp = wire::StoreOperation::Store;
        colorAttachment.StencilLoadOp = wire::LoadOperation::DontCare;
        colorAttachment.StencilStoreOp = wire::StoreOperation::DontCare;
        
        wire::AttachmentDesc depthAttachment{};
        depthAttachment.Format = wire::AttachmentFormat::SwapchainDepthDefault;
        depthAttachment.Usage = wire::AttachmentLayout::Depth;
        depthAttachment.PreviousAttachmentUsage = wire::AttachmentLayout::Undefined;
        depthAttachment.Samples = wire::AttachmentDesc::Count1Bit;
        depthAttachment.LoadOp = wire::LoadOperation::Clear;
        depthAttachment.StoreOp = wire::StoreOperation::Store;
        depthAttachment.StencilLoadOp = wire::LoadOperation::DontCare;
        depthAttachment.StencilStoreOp = wire::StoreOperation::DontCare;
        
        renderPassDesc.Attachments = { colorAttachment, depthAttachment };
        m_RenderPass = m_Renderer->createRenderPass(renderPassDesc, m_Renderer->getSwapchain(), "EngineLayer::m_RenderPass");

		m_ModelTexture = m_Renderer->createTexture2D("models/viking_room.png");
		Utils::LoadModel("models/viking_room.obj", m_ModelVertices, m_ModelIndices);

		m_ModelVertexBuffer = m_Renderer->createBuffer<wire::VertexBuffer>(m_ModelVertices.size() * sizeof(ModelVertex), m_ModelVertices.data());
		m_ModelIndexBuffer = m_Renderer->createBuffer<wire::IndexBuffer>(m_ModelIndices.size() * sizeof(uint32_t), m_ModelIndices.data());

		wire::InputLayout layout{};
		layout.VertexBufferLayout = {
			{ "POSITION", wire::ShaderDataType::Float4, sizeof(glm::vec4), offsetof(ModelVertex, Position) },
			{ "COLOR",    wire::ShaderDataType::Float4, sizeof(glm::vec4), offsetof(ModelVertex, Color)    },
			{ "TEXCOORD", wire::ShaderDataType::Float2, sizeof(glm::vec2), offsetof(ModelVertex, TexCoord) },
			{ "NORMAL",   wire::ShaderDataType::Float3, sizeof(glm::vec3), offsetof(ModelVertex, Normal) },
		};
		layout.Stride = sizeof(ModelVertex);
		layout.PushConstantInfos.push_back(
			wire::PushConstantInfo{
				.Size = sizeof(glm::mat4) * 2,
				.Offset = 0,
				.Shader = wire::ShaderType::Vertex
			}
		);
        wire::ShaderResourceInfo uniformResource = wire::ShaderResourceInfo{
            .Type = wire::ShaderResourceType::UniformBuffer,
            .Binding = 0,
            .Stage = wire::ShaderType::Vertex,
            .ArrayCount = 1
        };
		
        wire::ShaderResourceInfo imageSamplerResource = wire::ShaderResourceInfo{
            .Type = wire::ShaderResourceType::CombinedImageSampler,
            .Binding = 1,
            .Stage = wire::ShaderType::Pixel,
            .ArrayCount = 1
        };
        
        wire::ShaderResourceLayoutInfo layoutInfo = wire::ShaderResourceLayoutInfo{
            .Sets = {
                wire::ShaderResourceSetInfo{
                    .Resources = { uniformResource, imageSamplerResource }
                }
            }
        };
        
        m_ModelResourceLayout = m_Renderer->createShaderResourceLayout(layoutInfo);
        layout.ResourceLayout = m_ModelResourceLayout;
        
        m_ModelResource = m_Renderer->createShaderResource(0, m_ModelResourceLayout);

		wire::GraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.Layout = layout;
		pipelineDesc.ShaderPath = "shadercache://3DModel.hlsl";
		pipelineDesc.Topology = wire::PrimitiveTopology::TriangleList;
        pipelineDesc.RenderPass = m_RenderPass;

		m_ModelPipeline = m_Renderer->createGraphicsPipeline(pipelineDesc);

		wire::SamplerDesc samplerDesc{};
		samplerDesc.MinFilter = wire::SamplerFilter::Linear;
		samplerDesc.MagFilter = wire::SamplerFilter::Linear;
		samplerDesc.AddressModeU = wire::AddressMode::Repeat;
		samplerDesc.AddressModeV = wire::AddressMode::Repeat;
		samplerDesc.AddressModeW = wire::AddressMode::Repeat;
		samplerDesc.EnableAnisotropy = true;
		samplerDesc.MaxAnisotropy = m_Renderer->getMaxAnisotropy();
		samplerDesc.BorderColor = wire::BorderColor::IntOpaqueBlack;

		m_ModelSampler = m_Renderer->createSampler(samplerDesc);

        m_ModelUniformBuffer = m_Renderer->createBuffer<wire::UniformBuffer>(sizeof(glm::mat4) * 3);
        m_ModelUniformData = reinterpret_cast<glm::mat4*>(m_ModelUniformBuffer->map(sizeof(glm::mat4) * 3));

        m_ModelResource->update(m_ModelUniformBuffer, 0, 0);
		m_ModelResource->update(m_ModelTexture, m_ModelSampler, 1, 0);

		m_CommandLists.resize(m_Renderer->getNumFramesInFlight());
		for (size_t i = 0; i < m_Renderer->getNumFramesInFlight(); i++)
		{
			m_CommandLists[i] = m_Renderer->createCommandList();
		}
	}

	void EngineLayer::onDetach()
	{
        m_ModelUniformBuffer->unmap();
        delete m_ModelUniformBuffer;

		delete m_ModelSampler;
		delete m_ModelPipeline;
        delete m_ModelResource;
        delete m_ModelResourceLayout;
		delete m_ModelIndexBuffer;
		delete m_ModelVertexBuffer;
		delete m_ModelTexture;
		delete m_RenderPass;
	}

	void EngineLayer::onUpdate(float timestep)
	{
		wire::CommandList& commandList = m_CommandLists[m_Renderer->getFrameIndex()];

		struct
		{
			glm::mat4 Model;
			glm::mat4 View;
			glm::mat4 Proj;
		} uniformData{};

		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		uniformData.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		uniformData.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		uniformData.Proj = glm::perspective(glm::radians(45.0f), m_Renderer->getExtent().x / m_Renderer->getExtent().y, 0.1f, 10.0f);
		uniformData.Proj[1][1] *= -1.0f;

		std::memcpy(m_ModelUniformData, &uniformData, sizeof(glm::mat4) * 3);

		glm::vec2 extent = m_Renderer->getExtent();

		commandList.begin();
		commandList.beginRenderPass(m_RenderPass);

		commandList.bindPipeline(m_ModelPipeline);
		commandList.setViewport({ 0.0f, 0.0f }, extent, 0.0f, 1.0f);
		commandList.setScissor({ 0.0f, 0.0f }, extent);
        commandList.bindShaderResource(0, m_ModelResource);
		commandList.bindVertexBuffers({ m_ModelVertexBuffer->getBase() });
		commandList.bindIndexBuffer(m_ModelIndexBuffer->getBase());

		commandList.drawIndexed((uint32_t)m_ModelIndices.size());

		commandList.endRenderPass();
		commandList.end();

		m_Renderer->submitCommandList(commandList);
	}

	void EngineLayer::onEvent(wire::Event& event)
	{
	}

}
