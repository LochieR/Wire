#include "wrpch.h"
#include "Wire/Audio/Audio.h"

#include <RtAudio.h>

namespace Wire {

	RtAudio* s_Audio;
	SoundPlayer::CallbackData s_Data;

	int Audio::Init()
	{
		s_Audio = new RtAudio();

		if (s_Audio->getDeviceCount() == 0)
		{
			WR_CORE_ERROR("No audio devices found!");
			return 1;
		}

		s_Audio->showWarnings();

		return 0;
	}

	void Audio::Delete()
	{
		s_Audio->closeStream();
		delete s_Audio;
	}

	int SoundPlayer::Init()
	{
		uint32_t bufferSize = 4096;

		s_Audio = new RtAudio(RtAudio::WINDOWS_DS);
		if (!s_Audio)
		{
			WR_CORE_ERROR("Failed to create RtAudio.");
			return 1;
		}

		uint32_t deviceId = s_Audio->getDefaultOutputDevice();

		RtAudio::StreamParameters* outParam = new RtAudio::StreamParameters();

		outParam->deviceId = deviceId;
		outParam->nChannels = 2;

		s_Audio->openStream(outParam, NULL, RTAUDIO_FLOAT32, 44100,
			&bufferSize, rt_callback, &s_Data);

		s_Data.SampleRate = 44100;
		s_Data.FrameNumber = 44100;
		s_Data.ChannelNumber = outParam->nChannels;
		s_Data.Index = 0;
		s_Data.WaveFormTable = (float*)calloc(s_Data.ChannelNumber * s_Data.FrameNumber, sizeof(float));
		if (!s_Data.WaveFormTable)
		{
			WR_CORE_ERROR("Failed to allocate memory!");
			delete s_Audio;
			return 1;
		}

		return 0;
	}

	void SoundPlayer::Play(float freq)
	{
		for (uint32_t i = 0; i < s_Data.FrameNumber; i++)
		{
			float s = sin(i * M_PI * 2 * freq / s_Data.SampleRate);
			for (uint32_t x = 0; x < s_Data.ChannelNumber; x++)
			{
				s_Data.WaveFormTable[x * s_Data.ChannelNumber + x] = s;
			}
		}

		s_Audio->startStream();
	}

	void SoundPlayer::Stop()
	{
		s_Audio->stopStream();
	}

	int SoundPlayer::rt_callback(
		void* outBuf,
		void* inBuf,
		uint32_t frames,
		double streamTime,
		RtAudioStreamStatus status,
		void* userData
	)
	{
		(void)inBuf;
		float* buf = (float*)outBuf;
		uint32_t remainFrames;
		CallbackData* data = (CallbackData*)userData;

		remainFrames = frames;
		while (remainFrames > 0)
		{
			uint32_t sz = data->FrameNumber - data->Index;
			if (sz > remainFrames)
				sz = remainFrames;
			memcpy(buf, data->WaveFormTable + (data->Index * data->ChannelNumber),
				sz * data->ChannelNumber * sizeof(float));
			data->Index = (data->Index + sz) % data->FrameNumber;
			buf += sz * data->ChannelNumber;
			remainFrames -= sz;
		}
		return 0;
	}

	int FilePlayer::Play(const std::string& path)
	{
		return 0;
	}

}
