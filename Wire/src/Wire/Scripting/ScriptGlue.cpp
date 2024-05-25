#include "wrpch.h"
#include "ScriptGlue.h"

#include "Wire/Audio/AudioEngine.h"

#include <Coral/Assembly.hpp>

namespace Wire {

#define WR_ADD_INTERNAL_CALL(func) assembly.AddInternalCall("Wire.InternalCalls", #func, reinterpret_cast<void*>(func))

	void ScriptGlue::RegisterFunctions(void* assemblyPtr)
	{
		Coral::ManagedAssembly& assembly = *reinterpret_cast<Coral::ManagedAssembly*>(assemblyPtr);

		WR_ADD_INTERNAL_CALL(AudioProperties_GetSampleRate);
		WR_ADD_INTERNAL_CALL(AudioProperties_GetNumChannels);
		WR_ADD_INTERNAL_CALL(AudioEngine_WriteOutput);

		assembly.UploadInternalCalls();
	}

	uint32_t ScriptGlue::AudioProperties_GetSampleRate()
	{
		return AudioEngine::GetAudioProperties().SampleRate;
	}

	uint32_t ScriptGlue::AudioProperties_GetNumChannels()
	{
		return AudioEngine::GetAudioProperties().Channels;
	}

	void ScriptGlue::AudioEngine_WriteOutput(float leftChannel, float rightChannel)
	{
		AudioEngine::SetWrite(leftChannel, rightChannel);
	}

}
