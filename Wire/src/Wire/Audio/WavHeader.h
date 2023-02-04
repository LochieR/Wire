#pragma once

namespace Wire {

	class WaveFile
	{
	public:
		struct WaveHeader
		{
			std::array<char, 4> RiffHeader;
			uint32_t Size;
			std::array<char, 4> WaveHeader;

			std::array<char, 4> FmtHeader;
			uint32_t FmtChunkSize;
			uint16_t AudioFormat;
			uint16_t Channels;
			uint32_t SampleRate;
			uint32_t ByteRate;
			uint16_t SampleAlignment;
			uint16_t BitDepth;

			std::array<char, 4> DataHeader;
			uint32_t DataSize;
			uint32_t DataOffset;
		};
	public:
		static WaveHeader EncodeWaveHeader(uint8_t* waveHeader);
	};

}
