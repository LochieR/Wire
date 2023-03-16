#include "wrpch.h"
#include "Audio.h"

#include "WavHeader.h"

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

	RtAudio* Audio::m_AudioHandle;

	bool Audio::m_SceneRuntime = false;

	void Audio::Init()
	{
		m_AudioHandle = new RtAudio(RtAudio::WINDOWS_DS, RtAudioErrorCallback);

		m_AudioThread = std::thread(UpdateAudio);

		m_AudioThread.detach();
	}

	void Audio::Shutdown()
	{
		m_AudioThreadRunning = false;
	}

	void Audio::UpdateAudio()
	{
		bool streamStarted = false;

		RtAudio::StreamParameters streamParams;
		streamParams.deviceId = m_AudioHandle->getDefaultOutputDevice();
		streamParams.nChannels = 2;
		uint32_t sampleRate = 44100;
		uint32_t bufferFrames = 256;
		AudioData data;

		m_AudioHandle->openStream(&streamParams, NULL, RTAUDIO_FLOAT32, sampleRate, 
								  &bufferFrames, &AudioCallback, &data);

		data.SampleRate = 44100;
		data.nFrame = 44100;
		data.ChannelNumber = streamParams.nChannels;
		data.WaveFormTableIndex = 0;
		data.WaveFormTable = (float*)calloc(data.ChannelNumber * data.nFrame, sizeof(float));

		for (uint32_t i = 0; i < data.nFrame; i++)
		{
			float value = (float)sin(i * M_PI * 2 * 440 / data.SampleRate);
			for (uint32_t x = 0; x < data.ChannelNumber; x++)
				data.WaveFormTable[i * data.ChannelNumber + x] = value;
		}
		
		while (m_AudioThreadRunning)
		{
			if (m_SceneRuntime && !streamStarted)
			{
				m_AudioHandle->startStream();
				streamStarted = true;
			}
			if (!m_SceneRuntime && streamStarted)
			{
				m_AudioHandle->stopStream();
				streamStarted = false;
			}
		}

		m_AudioHandle->closeStream();

		WR_CORE_INFO("Deleting audio handle...");
		delete m_AudioHandle;
	}

}
