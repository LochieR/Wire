#pragma once

#include <string>

// Temporary
typedef unsigned int RtAudioStreamStatus;

namespace Wire {

	class Audio
	{
	public:
		static int Init();
		static void Delete();
	};

	class SoundPlayer
	{
	public:
		typedef struct
		{
			uint32_t SampleRate;
			uint32_t ChannelNumber;
			uint32_t FrameNumber;
			float* WaveFormTable;
			uint32_t Index;
		} CallbackData;
	public:
		static int Init();

		static void Play(float freq);
		static void Stop();
	private:
		static int rt_callback(
			void* outBuf,
			void* inBuf,
			uint32_t frames,
			double streamTime,
			RtAudioStreamStatus status,
			void* userData
		);
	};

	class FilePlayer
	{
	public:
		static int Play(const std::string& path);
	};

}
