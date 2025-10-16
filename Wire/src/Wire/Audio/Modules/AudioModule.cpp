#include "AudioModule.h"

#include "Wire/Core/Assert.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace wire {

	std::vector<AudioModule*> AudioGraphEvaluator::getEvaluationOrder(const std::vector<AudioModule*>& allModules)
	{
		std::unordered_set<AudioModule*> visited;
		std::unordered_set<AudioModule*> visiting;
		std::vector<AudioModule*> ordered;

		for (AudioModule* module : allModules)
		{
			depthFirstSearch(module, visited, visiting, ordered);
		}

		return ordered;
	}

	void AudioGraphEvaluator::depthFirstSearch(AudioModule* module, std::unordered_set<AudioModule*>& visited, std::unordered_set<AudioModule*>& visiting, std::vector<AudioModule*>& ordered)
	{
		if (visited.count(module))
			return;
		if (visiting.count(module))
		{
			WR_ASSERT(false, "Cyclic dependency in audio graph!");
		}

		visiting.insert(module);

		for (auto& input : module->getInputs())
		{
			if (input->ConnectedOutput && input->ConnectedOutput->Owner)
				depthFirstSearch(input->ConnectedOutput->Owner, visited, visiting, ordered);
		}

		visiting.erase(module);
		visited.insert(module);
		ordered.push_back(module);
	}

}
