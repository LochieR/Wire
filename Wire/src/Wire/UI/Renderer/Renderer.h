#pragma once

#include <functional>
#include <type_traits>
#include <filesystem>

#include "CommandBuffer.h"
#include "CommandList.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"
#include "Texture2D.h"
#include "FontCache.h"
#include "Font.h"
#include "ShaderCache.h"

namespace wire {

#define WR_FRAMES_IN_FLIGHT 2

	struct RendererDesc
	{
		RendererAPI API;
		ShaderCacheDesc ShaderCache;
		FontCacheDesc FontCache;
	};

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual glm::vec2 getExtent() const = 0;

		virtual uint32_t getFrameIndex() const = 0;
		virtual uint32_t getNumFramesInFlight() const = 0;

		virtual CommandBuffer& allocateCommandBuffer() = 0;
		virtual CommandBuffer& beginSingleTimeCommands() = 0;
		virtual void endSingleTimeCommands(CommandBuffer& commandBuffer) = 0;
		virtual void submitCommandBuffer(CommandBuffer& commandBuffer) = 0;

		virtual CommandList createCommandList() = 0;
		virtual void submitCommandList(const CommandList& commandList) = 0;

		virtual void submitResourceFree(std::function<void(Renderer*)>&& func) = 0;

		virtual GraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
		virtual VertexBuffer* createVertexBuffer(size_t size, const void* data = nullptr) = 0;
		virtual IndexBuffer* createIndexBuffer(size_t size, const void* data = nullptr) = 0;
		virtual StagingBuffer* createStagingBuffer(size_t size, const void* data = nullptr) = 0;
		virtual UniformBuffer* createUniformBuffer(size_t size, const void* data = nullptr) = 0;
		virtual Texture2D* createTexture2D(const std::filesystem::path& path) = 0;
		virtual Texture2D* createTexture2D(uint32_t* data, uint32_t width, uint32_t height) = 0;
		virtual Sampler* createSampler(const SamplerDesc& desc) = 0;
		virtual Font* createFont(const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) = 0;
		virtual Font* getFontFromCache(const std::filesystem::path& path) = 0;

		virtual ShaderCache& getShaderCache() = 0;
		virtual const ShaderCache& getShaderCache() const = 0;
		virtual FontCache& getFontCache() = 0;
		virtual const FontCache& getFontCache() const = 0;

		virtual float getMaxAnisotropy() const = 0;
	};

	Renderer* createRenderer(const RendererDesc& desc);

}
