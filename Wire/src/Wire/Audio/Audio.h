#pragma once

class RtAudio;

namespace Wire {

	class Audio
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

		static void SetSceneRuntime(bool runtime) { m_SceneRuntime = runtime; }

		static void UpdateAudio();
	private:
		static std::thread m_AudioThread;
		static bool m_AudioThreadRunning;

		static RtAudio* m_AudioHandle;

		static bool m_SceneRuntime;
	};

}
