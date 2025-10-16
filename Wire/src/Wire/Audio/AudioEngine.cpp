#include "AudioEngine.h"

#include "Wire/Core/Assert.h"

#include <portaudio.h>

#include <numbers>

namespace wire {

#define PA_RESULT(result, message) do { if (result != paNoError) { std::cerr << "Operation failed (" << message << ")\nPA error: " << Pa_GetErrorText(result) << std::endl; } } while (0)

	struct SineData
	{
		int FrameIndex = 0;
	};

	static float g_Frequency = 440.0f;

	static int audioCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData);

	void AudioEngine::init()
	{
		PaError result = Pa_Initialize();
		PA_RESULT(result, "Failed to initialize PortAudio!");

		SineData sineData{ 0 };

		PaStream* stream;
		result = Pa_OpenDefaultStream(
			&stream,
			0,
			1,
			paFloat32,
			44100,
			256,
			audioCallback,
			&sineData
		);
		PA_RESULT(result, "Failed to open PortAudio stream!");

		result = Pa_StartStream(stream);
		PA_RESULT(result, "Failed to start PortAudio stream!");

		while (true)
		{
			float newFreq = -1.0f;
			std::cin >> newFreq;
			if (newFreq <= 0.0f)
				break;
			g_Frequency = newFreq;
		}

		result = Pa_StopStream(stream);
		PA_RESULT(result, "Failed to stop PortAudio stream!");

		result = Pa_CloseStream(stream);
		PA_RESULT(result, "Failed to close PortAudio stream!");
	}

	void AudioEngine::shutdown()
	{
		PaError result = Pa_Terminate();
		PA_RESULT(result, "Failed to shutdown PortAudio!");
	}

	int audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
	{
		SineData* data = reinterpret_cast<SineData*>(userData);
		float* out = reinterpret_cast<float*>(outputBuffer);

		for (uint32_t i = 0; i < framesPerBuffer; i++)
		{
			out[i] = std::sin(2.0f * std::numbers::pi_v<float> * g_Frequency * data->FrameIndex / 44100) * 0.4f;
			data->FrameIndex++;
		}

		return paContinue;
	}

}
