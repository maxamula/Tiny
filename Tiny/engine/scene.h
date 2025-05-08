#pragma once
#include "entt.hpp"
#include "content/content.h"
#include "fx/shaderfx.h"
#include "taskflow/taskflow.hpp"
#include "camera.h"

#define TINY_SCENE_RING_BUFFER_COUNT 2

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
		entt::registry& GetRegistry() { return mRegistry; }
	_internal:
		//void Present(tf::Subflow& sf);

		entt::registry mRegistry;
		std::set<entt::entity> mRootEntities;
		
		/*struct RenderItem
		{

		};

		struct FrameSnapshot
		{

		};

		std::array<FrameSnapshot, TINY_SCENE_RING_BUFFER_COUNT> mRingBuffer;
		std::atomic<u8> mWriteIdx, mReadIdx;*/
	};
}