#pragma once

namespace Wire {

	class AudioEngine
	{
	public:
		struct AudioData
		{
			uint32_t SampleRate;
			uint32_t ChannelNumber;
			uint32_t nFrame;
			float* WaveFormTable;
			uint32_t WaveFormTableIndex;
		};
	public:
		static void Init();
		static void Shutdown();

		static void OnSceneStart() { m_SceneRuntime = true; }
		static void OnSceneStop() { m_SceneRuntime = false; }

		static void UpdateAudio();
	private:
		static std::thread m_AudioThread;
		static bool m_AudioThreadRunning;

		static bool m_SceneRuntime;
	};

}
