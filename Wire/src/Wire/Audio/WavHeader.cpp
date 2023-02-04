#include "wrpch.h"
#include "WavHeader.h"

namespace Wire {

	WaveFile::WaveHeader WaveFile::EncodeWaveHeader(uint8_t* waveHeader)
	{
		WaveHeader header{};
		uint8_t* headerCopy{ waveHeader };

		header.RiffHeader = {
			static_cast<char>(waveHeader[0]),
			static_cast<char>(waveHeader[1]),
			static_cast<char>(waveHeader[2]),
			static_cast<char>(waveHeader[3])
		};
		waveHeader += sizeof(std::array<char, 4>);

		header.Size = {
			static_cast<uint32_t>(waveHeader[3]) << 24 | static_cast<uint32_t>(waveHeader[2]) << 16 |
			static_cast<uint32_t>(waveHeader[1]) << 8 | static_cast<uint32_t>(waveHeader[0])
		};
		waveHeader += sizeof(uint32_t);

		header.WaveHeader = {
			static_cast<char>(waveHeader[0]),
			static_cast<char>(waveHeader[1]),
			static_cast<char>(waveHeader[2]),
			static_cast<char>(waveHeader[3])
		};
		waveHeader += sizeof(std::array<char, 4>);

		header.FmtHeader = { 
			static_cast<char>(waveHeader[0]),
			static_cast<char>(waveHeader[1]),
			static_cast<char>(waveHeader[2]),
			static_cast<char>(waveHeader[3])
		};
		waveHeader += sizeof(std::array<char, 4>);

		header.FmtChunkSize = {
			static_cast<uint32_t>(waveHeader[3]) << 24 | static_cast<uint32_t>(waveHeader[2]) << 16 |
			static_cast<uint32_t>(waveHeader[1]) <<  8 | static_cast<uint32_t>(waveHeader[0])
		};
		waveHeader += sizeof(uint32_t);

		header.AudioFormat = {
			/*static_cast<uint16_t>(waveHeader[1]) << 8 |
			static_cast<uint16_t>(waveHeader[0])*/
			static_cast<uint16_t>(waveHeader[1] << 8 | waveHeader[0])
		};
		waveHeader += sizeof(uint16_t);

		header.Channels = {
			/*static_cast<uint16_t>(waveHeader[1]) << 8 |
			static_cast<uint16_t>(waveHeader[0])*/
			static_cast<uint16_t>(waveHeader[1] << 8 | waveHeader[0])
		};
		waveHeader += sizeof(uint16_t);

		header.SampleRate = {
			static_cast<uint32_t>(waveHeader[3]) << 24 | static_cast<uint32_t>(waveHeader[2]) << 16 |
			static_cast<uint32_t>(waveHeader[1]) << 8 | static_cast<uint32_t>(waveHeader[0])
		};
		waveHeader += sizeof(uint32_t);

		header.ByteRate = {
			static_cast<uint32_t>(waveHeader[3]) << 24 | static_cast<uint32_t>(waveHeader[2]) << 16 |
			static_cast<uint32_t>(waveHeader[1]) << 8 | static_cast<uint32_t>(waveHeader[0]) };
		waveHeader += sizeof(uint32_t);

		header.SampleAlignment = {
			/*static_cast<uint16_t>(waveHeader[1]) << 8 |
			static_cast<uint16_t>(waveHeader[0])*/
			static_cast<uint16_t>(waveHeader[1] << 8 | waveHeader[0])
		};
		waveHeader += sizeof(uint16_t);

		header.BitDepth = {
			/*static_cast<uint16_t>(waveHeader[1]) << 8 |
			static_cast<uint16_t>(waveHeader[0])*/
			static_cast<uint16_t>(waveHeader[1] << 8 | waveHeader[0])
		};
		waveHeader += sizeof(uint16_t);

		constexpr std::array<char, 4> cmp{ 'd', 'a', 't', 'a' };
		for (;;)
		{
			header.DataHeader = {
				static_cast<char>(waveHeader[0]),
				static_cast<char>(waveHeader[1]),
				static_cast<char>(waveHeader[2]),
				static_cast<char>(waveHeader[3])
			};

			if (header.DataHeader == cmp)
			{
				waveHeader += sizeof(std::array<char, 4>);
				break;
			}
			else
				++waveHeader;
		}

		header.DataSize = {
			static_cast<uint32_t>(waveHeader[3]) << 24 | static_cast<uint32_t>(waveHeader[2]) << 16 |
			static_cast<uint32_t>(waveHeader[1]) << 8 | static_cast<uint32_t>(waveHeader[0])
		};
		waveHeader += sizeof(uint32_t);

		header.DataOffset = waveHeader - headerCopy;

		return header;
	}

}
