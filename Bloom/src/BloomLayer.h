#pragma once

#include "Wire/Core/Application.h"
#include "Wire/UI/Renderer/Renderer.h"

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
    private:
        wire::RenderPass* m_RenderPass;
        wire::GraphicsPipeline* m_GraphicsPipeline;
        
        std::array<wire::CommandList, WR_FRAMES_IN_FLIGHT> m_CommandLists;
    };

}
