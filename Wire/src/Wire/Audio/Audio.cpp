#include "wrpch.h"
#include "Audio.h"

#include "WavHeader.h"
#include "Modules.h"

#include <RtAudio.h>

namespace Wire {

	static void RtAudioErrorCallback(RtAudioErrorType errorType, const std::string& message)
	{
		switch (errorType)
		{
		case RTAUDIO_WARNING:
			WR_CORE_WARN(message);
			break;
		default:
			WR_CORE_ERROR(message);
			break;
		}
	}

	static int AudioCallback(void* outputBuffer, void* inputBuffer, uint32_t nFrames, 
							 double streamTime, RtAudioStreamStatus status, void* userData)
	{
		(void)inputBuffer;
		float* outBuffer = (float*)outputBuffer;
		uint32_t remainingFrames;
		Audio::AudioData* data = (Audio::AudioData*)userData;

		remainingFrames = nFrames;
		while (remainingFrames > 0)
		{
			uint32_t size = data->nFrame - data->WaveFormTableIndex;
			if (size > remainingFrames)
				size = remainingFrames;
			memcpy(outBuffer, data->WaveFormTable + (data->WaveFormTableIndex * data->ChannelNumber),
				size * data->ChannelNumber * sizeof(float));
			data->WaveFormTableIndex = (data->WaveFormTableIndex + size) % data->nFrame;
			outBuffer += size * data->ChannelNumber;
			remainingFrames -= size;
		}
		return 0;
	}
	
	std::thread Audio::m_AudioThread;
	bool Audio::m_AudioThreadRunning = true;

	bool Audio::m_SceneRuntime = false;

	void Audio::Init()
	{
		m_AudioThread = std::thread(UpdateAudio);
		m_AudioThread.detach();
	}

	void Audio::Shutdown()
	{
		m_AudioThreadRunning = false;
	}

	void Audio::UpdateAudio()
	{
		RtAudio* audio = new RtAudio(RtAudio::WINDOWS_DS, RtAudioErrorCallback);

		bool streamStarted = false;

		RtAudio::StreamParameters streamParams;
		streamParams.deviceId = audio->getDefaultOutputDevice();
		streamParams.nChannels = 2;
		uint32_t sampleRate = 44100;
		uint32_t bufferFrames = 512;
		AudioData data;

		audio->openStream(&streamParams, NULL, RTAUDIO_FLOAT32, sampleRate, 
								  &bufferFrames, &AudioCallback, &data);

		data.SampleRate = 44100;
		data.nFrame = 44100;
		data.ChannelNumber = streamParams.nChannels;
		data.WaveFormTableIndex = 0;
		data.WaveFormTable = (float*)calloc(data.ChannelNumber * data.nFrame, sizeof(float));

		AudioSettings settings;
		settings.NumChannels = data.ChannelNumber;
		settings.SampleRate = data.SampleRate;

		for (uint32_t i = 0; i < data.nFrame; i++)
		{
			settings.Index = i;
			float value = SineOscillator::GenerateAtFreq(440, settings);
			for (uint32_t x = 0; x < data.ChannelNumber; x++)
				data.WaveFormTable[i * data.ChannelNumber + x] = value;
		}
		
		while (m_AudioThreadRunning)
		{
			if (m_SceneRuntime && !streamStarted)
			{
				audio->startStream();
				streamStarted = true;
			}
			if (!m_SceneRuntime && streamStarted)
			{
				audio->stopStream();
				streamStarted = false;
			}
		}

		audio->closeStream();

		WR_CORE_INFO("Deleting audio handle...");
		delete audio;
	}

}
