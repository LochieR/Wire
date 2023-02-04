#include "wrpch.h"
#include "Audio.h"

#include "WavHeader.h"

#include <RtAudio.h>

namespace Wire {

	namespace Utils {

		static WaveFile::WaveHeader ReadWaveData(const std::string& filepath, uint32_t& outDataLength, std::vector<uint8_t>& outRawWaveData, uint8_t*& outRawWaveDataPointer)
		{
			WaveFile::WaveHeader waveHeader{};

			if (auto path{ std::filesystem::current_path() /= filepath }; std::filesystem::exists(path))
			{
				if (std::ifstream wav{ path, std::ios::binary }; wav.is_open())
				{
					uint8_t* buffer = new uint8_t[1024];
					wav.read((char*)buffer, 1024);
					waveHeader = WaveFile::EncodeWaveHeader(buffer);
					outDataLength = waveHeader.DataSize;
					outRawWaveData.reserve(waveHeader.DataSize);
					wav.seekg(waveHeader.DataOffset);
					wav.read(reinterpret_cast<char*>(&outRawWaveData[0]), waveHeader.DataSize);
					outRawWaveDataPointer = &outRawWaveData[0];
				}
				else
				{
					WR_CORE_ERROR("Cannot open file {0}!", filepath);
					return WaveFile::WaveHeader();
				}
			}
			else
			{
				WR_CORE_ERROR("File not found: {0}!", filepath);
				return WaveFile::WaveHeader();
			}

			return waveHeader;
		}

	}

	RtAudio* AudioPlayer::m_Audio = nullptr;

	static int AudioCallback(void* outBuffer, void* inBuffer, uint32_t frames, double time, RtAudioStreamStatus status, void* userData)
	{
		(void)inBuffer;
		float* buffer = (float*)outBuffer;
		uint32_t remainingFrames;
		AudioPlayer::CallbackData* data = (AudioPlayer::CallbackData*)userData;

		remainingFrames = frames;
		while (remainingFrames > 0)
		{
			uint32_t size = data->WaveNumber - data->WavesIndexInFrame;
			if (size > remainingFrames)
				size = remainingFrames;
			memcpy(buffer, data->Waves + (data->WavesIndexInFrame * data->ChannelNumber), size * data->ChannelNumber * sizeof(float));
			data->WavesIndexInFrame = (data->WavesIndexInFrame + size) % data->WaveNumber;
			buffer += size * data->ChannelNumber;
			remainingFrames -= size;
		}
		return 0;
	}

	static int AudioFileCallback(void* outBuffer, void* inBuffer, uint32_t frames, double time, RtAudioStreamStatus status, void* userData)
	{
		WaveFile::WaveHeader* waveHeader = (WaveFile::WaveHeader*)userData;


	}

	void AudioPlayer::Init()
	{
#ifdef WR_PLATFORM_WINDOWS
		m_Audio = new RtAudio(RtAudio::RTAUDIO_DUMMY);
#elif defined(WR_PLATFORM_MACOS)
		m_Audio = new RtAudio(RtAudio::MACOSX_CORE);
#elif defined(WR_PLAFORM_LINUX)
		m_Audio = new RtAudio(RtAudio::LINUX_ALSA);
#else
		#error Platform is not supported!
#endif

		WR_CORE_ASSERT(m_Audio->getDeviceCount() < 1);

		m_Audio->showWarnings(false);
	}

	void AudioPlayer::Shutdown()
	{
		delete m_Audio;
	}

	void AudioPlayer::PlayFile(const std::string& filepath)
	{
		uint32_t dataLength{};
		std::vector<uint8_t> rawWavData{};

		uint8_t* rawWavDataPtr;
	}

}
