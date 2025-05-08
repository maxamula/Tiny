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

	DirectX::XMMATRIX Scene::GetWorldMatrix(entt::entity entity)
	{
		ComponentTransform& transform = registry.get<ComponentTransform>(entity);
		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixScaling(transform.position.m128_f32[0], transform.position.m128_f32[1], transform.position.m128_f32[2]) *
			DirectX::XMMatrixRotationRollPitchYaw(transform.rotation.m128_f32[0], transform.rotation.m128_f32[1], transform.rotation.m128_f32[2]) *
			DirectX::XMMatrixTranslation(transform.scale.m128_f32[0], transform.scale.m128_f32[1], transform.scale.m128_f32[2]);

		ComponentHierarchy& hierarchy = registry.get<ComponentHierarchy>(entity);
		if (hierarchy.parent != entt::null)
		{
			DirectX::XMMATRIX parentMatrix = GetWorldMatrix(hierarchy.parent);
			worldMatrix = worldMatrix * parentMatrix;
		}

		return worldMatrix;
	}
}