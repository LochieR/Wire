module;

#include <functional>
#include <type_traits>
#include <filesystem>

export module wire.ui.renderer:renderer;

import :cmdBuffer;
import :graphicsPipeline;
import :buffer;
import :texture2D;
import :fontCache;
import :font;
import :shaderCache;

namespace wire {

	export struct RendererDesc
	{
		RendererAPI API;
		ShaderCacheDesc ShaderCache;
		FontCacheDesc FontCache;
	};

	export class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual uint32_t getFrameIndex() const = 0;

		virtual void draw(CommandBuffer commandBuffer, uint32_t vertexCount, uint32_t vertexOffset = 0) = 0;
		virtual void drawIndexed(CommandBuffer commandBuffer, uint32_t indexCount, uint32_t indexOffset = 0) = 0;

		virtual uint32_t getNumFramesInFlight() const = 0;

		virtual CommandBuffer allocateCommandBuffer() = 0;
		virtual CommandBuffer beginSingleTimeCommands() = 0;
		virtual void endSingleTimeCommands(CommandBuffer commandBuffer) = 0;
		virtual void beginCommandBuffer(CommandBuffer commandBuffer, bool renderPassStarted = true) = 0;
		virtual void endCommandBuffer(CommandBuffer commandBuffer) = 0;
		virtual void submitCommandBuffer(CommandBuffer commandBuffer) = 0;

		virtual void submitResourceFree(std::function<void(Renderer*)>&& func) = 0;

		virtual GraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
		virtual VertexBuffer* createVertexBuffer(size_t size, const void* data = nullptr) = 0;
		virtual IndexBuffer* createIndexBuffer(size_t size, const void* data = nullptr) = 0;
		virtual StagingBuffer* createStagingBuffer(size_t size, const void* data = nullptr) = 0;
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

	export Renderer* createRenderer(const RendererDesc& desc);

}
