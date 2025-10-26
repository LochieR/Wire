#pragma once

#include "IResource.h"
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
    
    class ComputePipeline : public IResource
    {
    public:
        virtual ~ComputePipeline() = default;
    };

}
