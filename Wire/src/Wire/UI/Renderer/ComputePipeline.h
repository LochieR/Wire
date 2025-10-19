#pragma once

#include "GraphicsPipeline.h"

#include <string>

namespace wire {

    struct ComputeInputLayout
    {
        std::vector<PushConstantInfo> PushConstantInfos;
        ShaderResourceLayout* ResourceLayout;
    };

    struct ComputePipelineDesc
    {
        std::string ShaderPath;
        ComputeInputLayout Layout;
    };
    
    class ComputePipeline
    {
    public:
        virtual ~ComputePipeline() = default;
    };

}
