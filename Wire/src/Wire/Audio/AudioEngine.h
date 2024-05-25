#pragma once

struct PaStreamCallbackTimeInfo;

typedef void PaStream;
typedef unsigned long PaStreamCallbackFlags;

#include "Wire/Core/Buffer.h"
#include "Wire/Scene/Scene.h"

namespace Wire {

	struct AudioProperties
	{
		uint32_t SampleRate;
		uint32_t Channels;
	};

	class AudioEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void Play();
		static void Stop();

		static void SetWrite(float leftChannel, float rightChannel);
		static void SetContext(Scene* scene) { s_Context = scene; }

		static const AudioProperties& GetAudioProperties() { return s_Properties; }
	private:
		static int AudioCallback(const void* inputBuffer, void* outputBuffer, unsigned long frames, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
	private:
		static AudioProperties s_Properties;

		static PaStream* s_Stream;

		inline static Scene* s_Context = nullptr;

		inline static float s_LeftChannel = 0.0f, s_RightChannel = 0.0f;
	};

}
