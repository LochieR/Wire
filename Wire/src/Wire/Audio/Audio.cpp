#include "wrpch.h"
#include "Audio.h"

#include <glm/gtc/type_ptr.hpp>

#include <AL/al.h>
#include <AL/alext.h>
#include <alc/alcmain.h>
#include "alhelpers.h"

#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include <minimp3_ex.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

namespace Wire {

	static ALCdevice* s_AudioDevice = nullptr;
	static mp3dec_t s_Mp3d;

	static uint8_t* s_AudioScratchBuffer;
	static uint32_t s_AudioScratchBufferSize = 10 * 1024 * 1024;

	enum class AudioFileFormat
	{
		None = 0,
		Ogg,
		MP3
	};

	static AudioFileFormat GetFileFormat(const std::filesystem::path& filepath)
	{
		std::string extension = filepath.extension().string();

		if (extension == ".ogg") return AudioFileFormat::Ogg;
		if (extension == ".mp3") return AudioFileFormat::MP3;

		WR_CORE_ASSERT(false);
		return AudioFileFormat::None;
	}

	static ALenum GetOpenALFormat(uint32_t channels)
	{
		switch (channels)
		{
			case 1: return AL_FORMAT_MONO16;
			case 2: return AL_FORMAT_STEREO16;
		}

		WR_CORE_ASSERT(false);
		return 0;
	}

	static void PrintAudioDeviceInfo()
	{
		WR_CORE_INFO("Audio Device Info:");
		WR_CORE_INFO("	Name: {}", s_AudioDevice->DeviceName);
		WR_CORE_INFO("	Sample Rate: {}", s_AudioDevice->Frequency);
		WR_CORE_INFO("	Max Sources: {}", s_AudioDevice->SourcesMax);
		WR_CORE_INFO("	Mono: {}", s_AudioDevice->NumMonoSources);
		WR_CORE_INFO("	Stereo: {}", s_AudioDevice->NumStereoSources);
	}

	void Audio::Init()
	{
		WR_CORE_ASSERT(InitAL(s_AudioDevice, nullptr, 0) == 0, "Audio device error!");

		PrintAudioDeviceInfo();

		mp3dec_init(&s_Mp3d);

		s_AudioScratchBuffer = new uint8_t[s_AudioScratchBufferSize];

		ALfloat listenerPos[] = { 0.0, 0.0, 0.0 };
		ALfloat listenerVel[] = { 0.0, 0.0, 0.0 };
		ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
		alListenerfv(AL_POSITION, listenerPos);
		alListenerfv(AL_VELOCITY, listenerVel);
		alListenerfv(AL_ORIENTATION, listenerOri);
	}

	AudioSource Audio::LoadAudioSource(const std::filesystem::path& filepath)
	{
		auto format = GetFileFormat(filepath);
		switch (format)
		{
			case AudioFileFormat::Ogg: return LoadAudioSourceOgg(filepath);
			case AudioFileFormat::MP3: return LoadAudioSourceMP3(filepath);
		}

		WR_CORE_ASSERT(false);
		return { 0, false, 0 };
	}

	void Audio::Play(const AudioSource& source)
	{
		alSourcePlay(source.m_SourceHandle);
	}

	void Audio::Stop(const AudioSource& source)
	{
		alSourceStop(source.m_SourceHandle);
	}

	void Audio::Pause(const AudioSource& source)
	{
		alSourcePause(source.m_SourceHandle);
	}

	AudioSource Audio::LoadAudioSourceOgg(const std::filesystem::path& filename)
	{
		FILE* f = fopen(filename.string().c_str(), "rb");

		OggVorbis_File vf;
		if (ov_open_callbacks(f, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
			WR_CORE_ERROR("Could not open ogg stream!");

		vorbis_info* vi = ov_info(&vf, -1);
		auto sampleRate = vi->rate;
		auto channels = vi->channels;
		auto alFormat = GetOpenALFormat(channels);

		uint64_t samples = ov_pcm_total(&vf, -1);
		float trackLength = (float)samples / (float)sampleRate;
		uint32_t bufferSize = 2 * channels * samples;

		WR_INFO("File Info for {}:", filename.string());
		WR_INFO("	Channels: {}", channels);
		WR_INFO("	Sample Rate: {}", sampleRate);
		WR_INFO("	Expected Size: {}", bufferSize);

		if (s_AudioScratchBufferSize < bufferSize)
		{
			s_AudioScratchBufferSize = bufferSize;
			delete[] s_AudioScratchBuffer;
			s_AudioScratchBuffer = new uint8_t[s_AudioScratchBufferSize];
		}

		uint8_t* oggBuffer = s_AudioScratchBuffer;
		uint8_t* bufferPtr = oggBuffer;
		int eof = 0;
		while (!eof)
		{
			int current_section;
			long length = ov_read(&vf, (char*)bufferPtr, 4096, 0, 2, 1, &current_section);
			if (length == 0)
				eof = 1;
			else if (length < 0)
			{
				if (length == OV_EBADLINK)
				{
					WR_CORE_ASSERT(false, "Corrupt bitstream section!");
					return { 0, false, 0 };
				}
			}
		}

		uint32_t size = bufferPtr - oggBuffer;

		WR_CORE_INFO("	Read {} bytes", size);

		ov_clear(&vf);
		fclose(f);

		ALuint buffer;
		alGenBuffers(1, &buffer);
		alBufferData(buffer, alFormat, oggBuffer, size, sampleRate);

		AudioSource result = { buffer, true, trackLength };
		alGenSources(1, &result.m_SourceHandle);
		alSourcei(result.m_SourceHandle, AL_BUFFER, buffer);

		WR_CORE_ASSERT(alGetError() == AL_NO_ERROR, "Failed to setup sound source!");

		return result;
	}

	AudioSource Audio::LoadAudioSourceMP3(const std::filesystem::path& filename)
	{
		mp3dec_file_info_t info;
		int loadResult = mp3dec_load(&s_Mp3d, filename.string().c_str(), &info, NULL, NULL);
		uint32_t size = info.samples * sizeof(mp3d_sample_t);

		auto sampleRate = info.hz;
		auto channels = info.channels;
		auto alFormat = GetOpenALFormat(channels);
		float lengthSeconds = size / (info.avg_bitrate_kbps * 1024.0f);

		ALuint buffer;
		alGenBuffers(1, &buffer);
		alBufferData(buffer, alFormat, info.buffer, size, sampleRate);

		AudioSource result = { buffer, true, lengthSeconds };
		alGenSources(1, &result.m_SourceHandle);
		alSourcei(result.m_SourceHandle, AL_BUFFER, buffer);

		WR_CORE_INFO("File Info for {}:", filename.string());
		WR_CORE_INFO("	Channels: {}", channels);
		WR_CORE_INFO("	Sample Rate: {}", sampleRate);
		WR_CORE_INFO("	Size: {} bytes", size);

		auto [mins, secs] = result.GetLengthMinutesAndSeconds();
		WR_CORE_INFO("	Length: {}m{}s", mins, secs);

		WR_CORE_ASSERT(alGetError() == AL_NO_ERROR, "Failed to setup sound source!");

		return result;
	}

	AudioSource::AudioSource(uint32_t handle, bool loaded, float length)
		: m_BufferHandle(handle), m_Loaded(loaded), m_TotalDuration(length)
	{
	}

	AudioSource::~AudioSource()
	{

	}

	void AudioSource::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		alSourcefv(m_SourceHandle, AL_POSITION, glm::value_ptr(position));
	}

	void AudioSource::SetGain(float gain)
	{
		m_Gain = gain;

		alSourcef(m_SourceHandle, AL_GAIN, gain);
	}

	void AudioSource::SetPitch(float pitch)
	{
		m_Pitch = pitch;

		alSourcef(m_SourceHandle, AL_PITCH, pitch);
	}

	void AudioSource::SetSpatial(bool spatial)
	{
		m_Spatial = spatial;

		alSourcei(m_SourceHandle, AL_SOURCE_SPATIALIZE_SOFT, spatial ? AL_TRUE : AL_FALSE);
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	}

	void AudioSource::SetLoop(bool loop)
	{
		m_Loop = loop;

		alSourcei(m_SourceHandle, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
	}

	std::pair<uint32_t, uint32_t> AudioSource::GetLengthMinutesAndSeconds() const
	{
		return { (uint32_t)(m_TotalDuration / 60.0f), (uint32_t)m_TotalDuration % 60 };
	}

	AudioSource AudioSource::LoadFromFile(const std::filesystem::path& file, bool spatial)
	{
		AudioSource result = Audio::LoadAudioSource(file);
		result.SetSpatial(spatial);
		return result;
	}

}
