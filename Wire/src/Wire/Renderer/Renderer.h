#pragma once

#include "Font.h"
#include "Shader.h"
#include "Buffer.h"
#include "Texture2D.h"
#include "Renderer2D.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"

#include "IResource.h"

#include "Wire/Core/Application.h"
#include "Wire/Core/Window.h"
#include "Wire/ImGui/ImGuiLayer.h"

#include <string>

#ifdef CreateFont
#undef CreateFont
#endif

namespace Wire {

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void Release() = 0;

		template<typename T>
		void Free(rbRef<T> ref)
		{
			Free(&ref);
		}

		virtual bool BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual uint32_t GetFrameIndex() const = 0;
		virtual uint32_t GetMaxFramesInFlight() const = 0;

		virtual Renderer2D& GetRenderer2D() = 0;

		virtual void Draw(rbRef<CommandBuffer> commandBuffer, uint32_t vertexCount) = 0;
		virtual void DrawIndexed(rbRef<CommandBuffer> commandBuffer, uint32_t indexCount) = 0;

		virtual rbRef<Shader> CreateShader(std::string_view path) = 0;
		virtual rbRef<VertexBuffer> CreateVertexBuffer(size_t size) = 0;
		virtual rbRef<IndexBuffer> CreateIndexBuffer(uint32_t* indices, uint32_t indexCount) = 0;
		virtual rbRef<StorageBuffer> CreateStorageBuffer(size_t size) = 0;
		virtual rbRef<StagingBuffer> CreateStagingBuffer(size_t size) = 0;
		virtual rbRef<Texture2D> CreateTexture2D(std::string_view path) = 0;
		virtual rbRef<Texture2D> CreateTexture2D(uint32_t* data, uint32_t width, uint32_t height) = 0;
		virtual rbRef<Font> CreateFont(std::string_view path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) = 0;
		virtual rbRef<Framebuffer> CreateFramebuffer(const FramebufferSpecification& spec) = 0;
		virtual ImGuiLayer* CreateImGuiLayer() = 0;

		virtual rbRef<CommandBuffer> AllocateCommandBuffer() = 0;
		virtual rbRef<CommandBuffer> BeginSingleTimeCommands() = 0;
		virtual void EndSingleTimeCommands(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void BeginCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void EndCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void BeginRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void EndRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void SubmitCommandBuffer(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void NextSubpass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void CopyBuffer(rbRef<CommandBuffer> commandBuffer, rbRef<StagingBuffer> srcBuffer, rbRef<StorageBuffer> dstBuffer) = 0;

		static Renderer* Create(Window& window);
	protected:
		virtual void Free(rbIRef* ref) = 0;
	};

}
