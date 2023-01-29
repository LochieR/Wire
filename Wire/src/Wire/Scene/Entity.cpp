#include "wrpch.h"
#include "Entity.h"

namespace Wire {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}
