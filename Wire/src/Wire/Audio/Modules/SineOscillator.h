#pragma once

#include "AudioModule.h"

namespace wire {

	class SineOscillator : public AudioModule
	{
	public:
		SineOscillator(float frequency, float sampleRate);
		virtual ~SineOscillator();

		float process() override;

		virtual std::vector<ModuleInput*>& getInputs() override { return m_Inputs; }
		virtual const std::vector<ModuleInput*>& getInputs() const override { return m_Inputs; }
		virtual std::vector<ModuleOutput*>& getOutputs() override { return m_Outputs; }
		virtual const std::vector<ModuleOutput*>& getOutputs() const override { return m_Outputs; }
	private:
		float m_SampleRate;
		float m_Phase = 0.0f;

		ModuleInput m_FrequencyInput{ .Name = "Frequency", .DefaultValue = 440.0f };
		ModuleOutput m_WaveformOutput{ .Name = "Waveform Output" };

		std::vector<ModuleInput*> m_Inputs;
		std::vector<ModuleOutput*> m_Outputs;
	};

}
