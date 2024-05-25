#pragma once

#include <glm/glm.hpp>

namespace Coral  {

	class String;

}

namespace Wire {

	class ScriptGlue
	{
	public:
		static void RegisterFunctions(void* assemblyPtr);
	private:
		static uint32_t AudioProperties_GetSampleRate();
		static uint32_t AudioProperties_GetNumChannels();
		static void AudioEngine_WriteOutput(float leftChannel, float rightChannel);
	};

}
