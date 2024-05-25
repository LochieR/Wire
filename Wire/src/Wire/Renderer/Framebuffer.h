#pragma once

#include "Buffer.h"
#include "CommandBuffer.h"
#include "IResource.h"

#include <vector>

namespace Wire {

	enum class AttachmentFormat
	{
		Default,
		RGBA,
		R32_SInt,
		R32_UInt,
		Depth
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 1, Height = 1;
		std::vector<AttachmentFormat> Attachments;
	};

	class Framebuffer : public IResource
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void BeginRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;
		virtual void EndRenderPass(rbRef<CommandBuffer> commandBuffer) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetColorAttachmentCount() const = 0;

		virtual void CopyAttachmentImageToBuffer(uint32_t attachmentIndex, rbRef<StagingBuffer> buffer) = 0;

		virtual void* GetRenderHandle() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};

}
