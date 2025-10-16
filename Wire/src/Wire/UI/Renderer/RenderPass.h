#pragma once

#include "Wire/Core/Assert.h"

#include <vector>

namespace wire {

	enum class AttachmentFormat
	{
		R8_UInt,
		R16_UInt,
		R32_UInt,
		R64_UInt,
		R8_SInt,
		R16_SInt,
		R32_SInt,
		R64_SInt,
		R8_UNorm,
		R16_UNorm,
		R32_SFloat,

		BGRA8_UNorm,
		RGBA8_UNorm,
		RGBA32_SFloat,

		D32_SFloat,
		// TBC
	};

	enum class LoadOperation : uint8_t
	{
		Load = 0,
		Clear = 1,
		DontCare = 2,
	};

	enum class StoreOperation : uint8_t
	{
		Store = 0,
		DontCare = 1,
	};

	enum class AttachmentUsage : uint8_t
	{
		Present = 0,
		Depth,
		TransferSrc
	};

	struct AttachmentDesc
	{
		enum SampleCount
		{
			Count1Bit = 0x00000001,
			Count2Bit = 0x00000002,
			Count4Bit = 0x00000004,
			Count8Bit = 0x00000008,
			Count16Bit = 0x00000010,
			Count32Bit = 0x00000020,
			Count64Bit = 0x00000040,
		};

		AttachmentFormat Format;
		AttachmentUsage Usage;
		SampleCount Samples;
		LoadOperation LoadOp;
		StoreOperation StoreOp;
		LoadOperation StencilLoadOp;
		StoreOperation StencilStoreOp;
	};

	struct RenderPassDesc
	{
		std::vector<AttachmentDesc> Attachments;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
	};

}
