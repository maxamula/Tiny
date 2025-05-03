#pragma once
#include "entt.hpp"
#include "content/content.h"
#include "fx/shaderfx.h"

#include <DirectXMath.h>

namespace tiny
{
	struct ComponentHierarchy
	{
	_internal:
		entt::entity parent = entt::null;
		std::vector<entt::entity> children;
		std::string tag;
	};

	struct TINYFX_API Scene
	{
		entt::entity CreateObject(entt::entity entity, const std::string& tag);
		entt::entity CreateObject(const std::string& tag);
	_internal:
		entt::registry registry;
		std::set<entt::entity> rootEntities;
	};

	struct TINYFX_API ComponentRenderItem
	{
		Mesh mesh;
		fx::MeshTechnique technique;
	};

	struct TINYFX_API ComponentTransform
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
}