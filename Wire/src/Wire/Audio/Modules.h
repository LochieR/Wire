#pragma once

namespace Wire {

	struct AudioSettings
	{
		uint32_t SampleRate;
		uint32_t Index;
		uint32_t NumChannels;
	};

	class SineOscillator
	{
	public:
		static float GenerateAtFreq(float freq, const AudioSettings& settings)
		{
			return (float)sin(settings.Index * M_PI * settings.NumChannels * freq / settings.SampleRate);
		}
	};

}
