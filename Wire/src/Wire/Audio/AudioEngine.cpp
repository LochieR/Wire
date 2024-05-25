#include "wrpch.h"
#include "AudioEngine.h"

#include "Wire/Core/Base.h"
#include "Wire/Utils/PlatformUtils.h"

#include "Wire/Scene/Module.h"

#include "Wire/Scripting/ScriptEngine.h"
#include <Coral/ManagedObject.hpp>

#include <portaudio.h>
#include <../src/common/pa_debugprint.h>

#include <bit>
#include <thread>
#include <numbers>

namespace Wire {

#define WR_PA_CHECK(paError) { if (paError != paNoError) { WR_ERROR("[", paError, "] ", Pa_GetErrorText(paError)); WR_ASSERT(false); } }

	AudioProperties AudioEngine::s_Properties{};
	PaStream* AudioEngine::s_Stream = nullptr;

	void AudioEngine::Init()
	{
		WR_TAG("AudioEngine:PortAudio");

		s_Properties.SampleRate = 44100;
		s_Properties.Channels = 2;

		PaUtil_SetDebugPrintFunction([](const char* msg) { WR_TEMP_TAG("AudioEngine:PortAudio"); WR_INFO(msg); });

		PaError error = Pa_Initialize();
		WR_PA_CHECK(error);

		error = Pa_OpenDefaultStream(&s_Stream, 0, s_Properties.Channels, paFloat32, s_Properties.SampleRate, 60, &AudioCallback, nullptr);
		WR_PA_CHECK(error);

		Play();
	}

	void AudioEngine::Shutdown()
	{
		Stop();

		WR_TAG("AudioEngine:PortAudio");

		PaError error = Pa_CloseStream(s_Stream);
		WR_PA_CHECK(error);

		Pa_Terminate();
	}

	static bool s_Created = false;
	static std::thread s_AudioThread;
	static bool s_Play = false;

	static void AudioThread(PaStream* stream)
	{
		PaError error = Pa_StartStream(stream);
		WR_PA_CHECK(error);

		while (s_Play);

		error = Pa_StopStream(stream);
		WR_PA_CHECK(error);
	}

	void AudioEngine::Play()
	{
		s_Play = true;

		if (!s_Created)
		{
			s_AudioThread = std::thread(AudioThread, s_Stream);
			s_AudioThread.detach();

			s_Created = true;
		}
	}

	void AudioEngine::Stop()
	{
		s_Play = false;
	}

	void AudioEngine::SetWrite(float leftChannel, float rightChannel)
	{
		s_LeftChannel = leftChannel;
		s_RightChannel = rightChannel;
	}

	int AudioEngine::AudioCallback(const void* inputBuffer, void* outputBuffer, unsigned long frames, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
	{
		float* out = reinterpret_cast<float*>(outputBuffer);

		if (!s_Context)
			return 0;

		static UUID sineUUID = 0;
		static entt::entity id = entt::null;
		if (sineUUID == 0)
		{
			for (const auto& [uuid, moduleID] : *s_Context)
			{
				sineUUID = uuid;
				id = moduleID;
			}
		}
		static void(*update)() = ScriptEngine::GetUpdateMethod(sineUUID);

		static float freq = 440.0f;
		//static_cast<Controls::SliderControl<float>*>(Module{ id, s_Context }.GetComponent<NumberControlsComponent>().Controls[0])->SetValue(freq);
		if (freq >= 45.0f)
			freq -= 0.5f;

		for (uint32_t i = 0; i < frames; i++)
		{
			s_LeftChannel = 0.0f;
			s_RightChannel = 0.0f;

			update();

			*out++ = s_LeftChannel;
			*out++ = s_RightChannel;
		}

		return 0;
	}

}