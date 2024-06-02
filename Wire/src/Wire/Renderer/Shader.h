#pragma once

#include "Buffer.h"
#include "Framebuffer.h"
#include "GraphicsPipeline.h"
#include "IResource.h"

#include <filesystem>

namespace Wire {

	class Shader : public IResource
	{
	public:
		virtual ~Shader() = default;

		virtual void Reload() = 0;
		virtual rbRef<GraphicsPipeline> CreatePipeline(const InputLayout& layout, PrimitiveTopology topology, bool multiSample) = 0;
		virtual rbRef<GraphicsPipeline> CreatePipeline(const InputLayout& layout, PrimitiveTopology topology, bool multiSample, rbRef<Framebuffer> framebuffer) = 0;

		virtual const std::filesystem::path& GetFilepath() const = 0;
	};

}
