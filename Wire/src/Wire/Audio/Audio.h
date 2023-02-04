#pragma once

class RtAudio;

namespace Wire {

	class AudioPlayer
	{
	public:
		typedef struct
		{
			uint32_t SampleRate;
			uint32_t ChannelNumber;
			uint32_t WaveNumber;
			float* Waves;
			uint32_t WavesIndexInFrame;
		} CallbackData;
	public:
		static void Init();
		static void Shutdown();

		static void PlayFile(const std::string& filepath);
	private:
		static RtAudio* m_Audio;
	};

}
