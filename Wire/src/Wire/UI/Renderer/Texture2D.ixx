module;

#include <cstdint>

export module wire.ui.renderer:texture2D;

import wire.core.uuid;

namespace wire {

	export class Texture2D
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual UUID getUUID() const = 0;
	};

	export enum class SamplerFilter
	{
		Nearest = 0,
		Linear = 1
	};

	export enum class AddressMode
	{
		Repeat = 0,
		MirroredRepeat = 1,
		ClampToEdge = 2,
		ClampToBorder = 3,
		MirrorClampToEdge = 4
	};
	
	export enum class BorderColor
	{
		FloatTransparentBlack = 0,
		IntTransparentBlack = 1,
		FloatOpaqueBlack = 2,
		IntOpaqueBlack = 3,
		FloatOpaqueWhite = 4,
		IntOpaqueWhite = 5
	};

	export struct SamplerDesc
	{
		SamplerFilter MinFilter;
		SamplerFilter MagFilter;
		AddressMode AddressModeU;
		AddressMode AddressModeV;
		AddressMode AddressModeW;
		bool EnableAnisotropy;
		float MaxAnisotropy;
		BorderColor BorderColor;
	};

	export class Sampler
	{
	public:
		virtual ~Sampler() = default;
	};

}
