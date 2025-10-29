#pragma once

#include "Wire/Core/Application.h"
#include "Wire/Renderer/Device.h"

#include <glm/glm.hpp>

#include <array>

namespace bloom {

    class BloomLayer : public wire::Layer
    {
    public:
        BloomLayer() = default;
        ~BloomLayer() = default;
        
        virtual void onAttach() override;
        virtual void onDetach() override;
        virtual void onImGuiRender() override;
        virtual void onUpdate(float timestep) override;
    private:
        std::shared_ptr<wire::Device> m_Device;

        float m_Threshold = 1.0f;
        float m_Intensity = 2.0f;
        float m_BloomStrength = 0.2f;

        uint32_t m_MipCount;

        std::shared_ptr<wire::RenderPass> m_ColorRenderPass = nullptr;
        std::shared_ptr<wire::GraphicsPipeline> m_ColorPipeline = nullptr;
        std::shared_ptr<wire::Buffer> m_ColorVertexBuffer = nullptr;
        std::shared_ptr<wire::Framebuffer> m_ColorFramebuffer = nullptr;
        std::shared_ptr<wire::Buffer> m_IndexBuffer = nullptr;

        std::shared_ptr<wire::ComputePipeline> m_BrightPassDownsamplePipeline = nullptr;
        std::shared_ptr<wire::Framebuffer> m_BrightPassFramebuffer = nullptr;
        std::shared_ptr<wire::ShaderResourceLayout> m_BrightPassResourceLayout = nullptr;
        std::vector<std::shared_ptr<wire::ShaderResource>> m_BrightPassResources;
        std::shared_ptr<wire::Sampler> m_BrightPassSampler = nullptr;

        std::shared_ptr<wire::ComputePipeline> m_BlurPipeline = nullptr;
        std::shared_ptr<wire::Framebuffer> m_BlurIntermediateFramebuffer = nullptr;
        std::shared_ptr<wire::Framebuffer> m_BlurFramebuffer = nullptr;
        std::shared_ptr<wire::ShaderResourceLayout> m_BlurResourceLayout = nullptr;
        std::vector<std::array<std::shared_ptr<wire::ShaderResource>, 2>> m_BlurResources;

        std::shared_ptr<wire::ComputePipeline> m_UpsamplePipeline = nullptr;
        std::shared_ptr<wire::Framebuffer> m_UpsampleFramebuffer = nullptr;
        std::shared_ptr<wire::ShaderResourceLayout> m_UpsampleResourceLayout = nullptr;
        std::vector<std::shared_ptr<wire::ShaderResource>> m_UpsampleResources;
        std::shared_ptr<wire::Sampler> m_UpsampleLowSampler = nullptr;
        std::shared_ptr<wire::Sampler> m_UpsampleHighSampler = nullptr;

        std::shared_ptr<wire::RenderPass> m_CombineRenderPass = nullptr;
        std::shared_ptr<wire::GraphicsPipeline> m_CombinePipeline = nullptr;
        std::shared_ptr<wire::ShaderResourceLayout> m_CombineResourceLayout = nullptr;
        std::shared_ptr<wire::ShaderResource> m_CombineResource = nullptr;
        std::shared_ptr<wire::Sampler> m_CombineSampler = nullptr;
        
        std::array<wire::CommandList, WR_FRAMES_IN_FLIGHT> m_CommandLists;
    };

}
