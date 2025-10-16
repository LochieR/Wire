#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace wire {

	class AudioModule;
	struct ModuleInput;

	struct ModuleOutput
	{
		std::string Name;

		std::vector<ModuleInput*> ModuleInputs;
		float Value = 0.0f;

		AudioModule* Owner = nullptr;
	};

	struct ModuleInput
	{
		std::string Name;

		ModuleOutput* ConnectedOutput = nullptr;
		float DefaultValue = 0.0f;
		float Value = 0.0f;

		void updateValue()
		{
			Value = ConnectedOutput ? ConnectedOutput->Value : DefaultValue;
		}
	};

	class AudioModule
	{
	public:
		virtual ~AudioModule() = default;

		virtual float process() = 0;

		virtual std::vector<ModuleInput*>& getInputs() = 0;
		virtual const std::vector<ModuleInput*>& getInputs() const = 0;
		virtual std::vector<ModuleOutput*>& getOutputs() = 0;
		virtual const std::vector<ModuleOutput*>& getOutputs() const = 0;
	};

	class AudioGraphEvaluator
	{
	public:
		static std::vector<AudioModule*> getEvaluationOrder(const std::vector<AudioModule*>& allModules);
	private:
		static void depthFirstSearch(AudioModule* module, std::unordered_set<AudioModule*>& visited, std::unordered_set<AudioModule*>& visiting, std::vector<AudioModule*>& ordered);
	};

}
