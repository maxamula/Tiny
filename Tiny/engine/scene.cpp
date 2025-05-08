#include "scene.h"

namespace tiny
{
	entt::entity Scene::CreateObject(entt::entity entity, const std::string& tag)
	{
		auto child = mRegistry.create();
		ComponentHierarchy& hierarchy = mRegistry.emplace<ComponentHierarchy>(child);
		hierarchy.parent = entity;
		hierarchy.tag = tag;
		mRegistry.get<ComponentHierarchy>(entity).children.push_back(child);
		return child;
	}

	entt::entity Scene::CreateObject(const std::string& tag)
	{
		auto entity = mRegistry.create();
		ComponentHierarchy& hierarchy = mRegistry.emplace<ComponentHierarchy>(entity);
		hierarchy.tag = tag;
		mRootEntities.insert(entity);
		return entity;
	}

	DirectX::XMMATRIX Scene::GetWorldMatrix(entt::entity entity)
	{
		ComponentTransform& transform = mRegistry.get<ComponentTransform>(entity);
		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixScaling(transform.position.m128_f32[0], transform.position.m128_f32[1], transform.position.m128_f32[2]) *
			DirectX::XMMatrixRotationRollPitchYaw(transform.rotation.m128_f32[0], transform.rotation.m128_f32[1], transform.rotation.m128_f32[2]) *
			DirectX::XMMatrixTranslation(transform.scale.m128_f32[0], transform.scale.m128_f32[1], transform.scale.m128_f32[2]);

		ComponentHierarchy& hierarchy = mRegistry.get<ComponentHierarchy>(entity);
		if (hierarchy.parent != entt::null)
		{
			DirectX::XMMATRIX parentMatrix = GetWorldMatrix(hierarchy.parent);
			worldMatrix = worldMatrix * parentMatrix;
		}

		return worldMatrix;
	}

	/*void Scene::Present(tf::Subflow& sf)
	{
		auto view = mRegistry.view<ComponentGeometry, ComponentMaterial>().each();
		sf.for_each(view.begin(), view.end(), [](auto& entry)
		{
			std::cout << "Processing object: " << std::endl;
		});

		mWriteIdx = (mWriteIdx + 1) % TINY_SCENE_RING_BUFFER_COUNT;
		mReadIdx = (mReadIdx + 1) % TINY_SCENE_RING_BUFFER_COUNT;
	}*/
}