#include "wrpch.h"
#include "Module.h"

namespace Wire {

	Module::Module(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}
