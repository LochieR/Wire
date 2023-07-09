#pragma once

#include "Wire/Scene/Scene.h"

#include <map>
#include <vector>
#include <typeindex>

namespace Wire {

	class Panel
	{
	public:
		virtual ~Panel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void SetContext(const Ref<Scene>& scene) = 0;

		virtual bool* GetOpen() = 0;
	};

	class PanelStack
	{
	public:
		PanelStack() = default;

		template<typename T>
		void Add(Panel* panel)
		{
			m_Panels.push_back(panel);
			m_TypeIndices[std::type_index(typeid(T))] = (int)m_Panels.size() - 1;
			//return panel;
		}

		template<typename T>
		Panel* Get()
		{
			auto it = m_TypeIndices.find(std::type_index(typeid(T)));
			WR_CORE_ASSERT(it != m_TypeIndices.end());

			int index = it->second;
			return m_Panels[index];
		}

		Panel& operator[](int index)
		{
			return *(m_Panels[index]);
		}

		std::vector<Panel*>::iterator begin() { return m_Panels.begin(); }
		std::vector<Panel*>::iterator end() { return m_Panels.end(); }
		std::vector<Panel*>::const_iterator begin() const { return m_Panels.begin(); }
		std::vector<Panel*>::const_iterator end() const { return m_Panels.end(); }
	private:
		std::vector<Panel*> m_Panels;

		std::map<std::type_index, int> m_TypeIndices;
	};

}
