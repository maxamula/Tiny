#include "scene.h"

namespace tiny
{
	entt::entity Scene::CreateObject(entt::entity entity, const std::string& tag)
	{
		auto child = registry.create();
		ComponentHierarchy& hierarchy = registry.emplace<ComponentHierarchy>(child);
		hierarchy.parent = entity;
		hierarchy.tag = tag;
		registry.get<ComponentHierarchy>(entity).children.push_back(child);
		return child;
	}

	entt::entity Scene::CreateObject(const std::string& tag)
	{
		auto entity = registry.create();
		ComponentHierarchy& hierarchy = registry.emplace<ComponentHierarchy>(entity);
		hierarchy.tag = tag;
		rootEntities.insert(entity);
		return entity;
	}
}