#pragma once

#include <glm/glm.hpp>

#include <string>
#include <filesystem>

namespace Wire {

	class AudioSource
	{
	public:
		AudioSource() = default;
		~AudioSource();

		bool IsLoaded() const { return m_Loaded; }

		void SetPosition(const glm::vec3& position);
		void SetGain(float gain);
		void SetPitch(float pitch);
		void SetSpatial(bool spatial);
		void SetLoop(bool loop);

		std::pair<uint32_t, uint32_t> GetLengthMinutesAndSeconds() const;

		static AudioSource LoadFromFile(const std::filesystem::path& file, bool spatial = false);
	private:
		AudioSource(uint32_t handle, bool loaded, float length);
	private:
		uint32_t m_BufferHandle;
		uint32_t m_SourceHandle;
		bool m_Loaded = false;
		bool m_Spatial = false;

		float m_TotalDuration;

		glm::vec3 m_Position = glm::vec3(0.0f);
		float m_Gain = 1.0f;
		float m_Pitch = 1.0f;
		bool m_Loop = false;

		friend class Audio;

	};

	class Audio
	{
	public:
		static void Init();

		static AudioSource LoadAudioSource(const std::filesystem::path& filepath);
		static void Play(const AudioSource& source);
		static void Stop(const AudioSource& source);
		static void Pause(const AudioSource& source);
	private:
		static AudioSource LoadAudioSourceOgg(const std::filesystem::path& filename);
		static AudioSource LoadAudioSourceMP3(const std::filesystem::path& filename);
	};

}
