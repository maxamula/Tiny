#pragma once
#include "entt.hpp"
#include "content/content.h"
#include "fx/shaderfx.h"
#include "taskflow/taskflow.hpp"

#define TINY_SCENE_RING_BUFFER_COUNT 2

namespace tiny
{
	using TagDestroyed = entt::tag<"TagDestroyed"_hs>;
	using ComponentGeometry = Mesh;
	using ComponentMaterial = fx::MeshTechnique;

	struct ComponentHierarchy
	{
	_internal:
		entt::entity parent = entt::null;
		std::set<entt::entity> children;
		std::string tag;
	};

	struct TINYFX_API ComponentTransform
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR rotation;
		DirectX::XMVECTOR scale;
	};

	struct TINYFX_API ComponentCamera
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR lookDir;
		f32 fov;
		f32 nearZ;
		f32 farZ;
		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRect;
	};

	struct TINYFX_API LightCommon
	{
		DirectX::XMFLOAT3 color;
		f32 intensity;
	};

	struct TINYFX_API ComponentPointLight : LightCommon
	{
		f32 range;
		f32 InvSqrRange() const { return 1.0f / std::pow(range, 2); }
	};

	class IScript
	{
	public:
		virtual ~IScript() = default;
		virtual void OnUpdate(entt::entity entity, f32 deltaTime) = 0;
		virtual void OnFixedUpdate(entt::entity entity) = 0;
	};

	struct TINYFX_API ComponentScript
	{
		std::unique_ptr<IScript> script;
	};

	struct TINYFX_API ComponentDirectionalLight : LightCommon
	{};

	using SceneObserver = entt::constness_as_t<entt::storage_type_t<entt::reactive, entt::entity, std::allocator<entt::reactive>>, entt::reactive>;

	class TINYFX_API Scene
	{
	public:
		Scene();

		entt::entity CreateObject(const entt::entity object, const std::string& tag);
		entt::entity CreateObject(const std::string& tag);
		
		template <typename T>
		const T& Get(entt::entity entity) const
		{
			return mRegistry.get<T>(entity);
		}

		template <typename T, typename F>
		T& Modify(entt::entity entity, F&& fn)
		{
			return mRegistry.patch<T>(entity, std::forward<F>(fn));
		}

		template <typename T, typename... Args>
		const T& Emplace(entt::entity entity, Args&&... args)
		{
			return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
		}

		void Destroy(entt::entity entity)
		{
			mRegistry.emplace_or_replace<TagDestroyed>(entity);
		}

	_internal:
		void FinalizeLogicfFrame();

		entt::registry mRegistry;
		std::set<entt::entity> mRootEntities;

		struct RenderItem
		{
			entt::entity entity;
			DirectX::XMMATRIX world;
			Mesh mesh;
			fx::MeshTechnique material;
		};

		SceneObserver& mDirty;
		SceneObserver& mPendingRemoveStorage;
		std::vector<RenderItem> mRenderList;
		std::unordered_map<entt::entity, u32> mRenderIndex;

		struct CameraItem
		{
			entt::entity entity;
			DirectX::XMFLOAT3 position;
			DirectX::XMMATRIX view;
			f32 fov, nearZ, farZ;
		};

		SceneObserver& mCameraDirty;
		std::vector<CameraItem> mCameras;
		std::unordered_map<entt::entity, u32> mCameraIndex;

		enum class LightType : u8
		{
			LightType_Directional = 0,
			LightType_Point = 1,
			LightType_Count
		};

		struct GPULight
		{
			LightType type;
			DirectX::XMFLOAT3 radiance;
			DirectX::XMFLOAT3 position;
			f32 invSqrRange;
			DirectX::XMFLOAT3 direction;
		};

		SceneObserver& mLightDirty;
		std::vector<GPULight> mLights;
		std::unordered_map<entt::entity, u32> mLightIndex;
	};
}