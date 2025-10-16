#include "SineOscillator.h"

#include <cmath>
#include <numbers>

namespace wire {

	SineOscillator::SineOscillator(float frequency, float sampleRate)
		: m_SampleRate(sampleRate)
	{
		m_WaveformOutput.Owner = this;

		m_Inputs = {
			&m_FrequencyInput
		};

		m_Outputs = {
			&m_WaveformOutput
		};
	}

	SineOscillator::~SineOscillator()
	{
	}

	float SineOscillator::process()
	{
		for (ModuleInput* input : m_Inputs)
			input->updateValue();

		float value = std::sin(m_Phase);
		m_Phase += 2.0f * std::numbers::pi_v<float> * m_FrequencyInput.Value / m_SampleRate;
		if (m_Phase > 2.0f * std::numbers::pi_v<float>)
			m_Phase -= 2.0f * std::numbers::pi_v<float>;

		return value;
	}

}
