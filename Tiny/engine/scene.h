#pragma once
#include "entt.hpp"
#include "content/content.h"
#include "fx/shaderfx.h"
#include "camera.h"

namespace tiny
{
	using ComponentGeometry = Mesh;
	using ComponentMaterial = fx::MeshTechnique;

	struct ComponentHierarchy
	{
	_internal:
		entt::entity parent = entt::null;
		std::vector<entt::entity> children;
		std::string tag;
	};

	struct TINYFX_API ComponentTransform
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotation;
		DirectX::XMVECTOR scale;
	};

	struct TINYFX_API Scene
	{
		entt::entity CreateObject(entt::entity entity, const std::string& tag);
		entt::entity CreateObject(const std::string& tag);

		DirectX::XMMATRIX GetWorldMatrix(entt::entity entity);
		entt::registry& GetRegistry() { return registry; }
	_internal:
		entt::registry registry;
		std::set<entt::entity> rootEntities;
	};
}