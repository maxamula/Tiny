#include "scene.h"

namespace tiny
{
	namespace
	{
		DirectX::XMMATRIX GetObjectWorldMat(entt::entity object, const entt::registry& reg)
		{
			const ComponentTransform& transform = reg.get<ComponentTransform>(object);
			DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
			auto scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
			worldMatrix *= DirectX::XMMatrixScaling(DirectX::XMVectorGetX(scale), DirectX::XMVectorGetY(scale), DirectX::XMVectorGetZ(scale))*
				DirectX::XMMatrixRotationQuaternion(transform.rotation) *
				DirectX::XMMatrixTranslation(DirectX::XMVectorGetX(transform.position), DirectX::XMVectorGetY(transform.position), DirectX::XMVectorGetZ(transform.position));

			const ComponentHierarchy& hierarchy = reg.get<ComponentHierarchy>(object);
			if (hierarchy.parent != entt::null)
			{
				DirectX::XMMATRIX parentMatrix = GetObjectWorldMat(hierarchy.parent, reg);
				worldMatrix = worldMatrix * parentMatrix;
			}
			return worldMatrix;
		}
	} // namespace anonymous

	Scene::Scene()
		: mRegistry(),
		mDirty(mRegistry.storage<entt::reactive>("DirtyObserver"_hs)),
		mPendingRemoveStorage(mRegistry.storage<entt::reactive>("DestroyObserver"_hs)),
		mCameraDirty(mRegistry.storage<entt::reactive>("CameraDirtyObserver"_hs)),
		mLightDirty(mRegistry.storage<entt::reactive>("LightDirtyObserver"_hs))
	{
		mDirty.on_construct<ComponentMaterial>()
			.on_update<ComponentMaterial>()
			.on_destroy<ComponentMaterial>()
			.on_construct<ComponentTransform>()
			.on_update<ComponentTransform>();

		mPendingRemoveStorage.on_construct<TagDestroyed>();

		mCameraDirty.on_construct<ComponentCamera>()
			.on_update<ComponentCamera>()
			.on_destroy<ComponentCamera>();
	}

	entt::entity Scene::CreateObject(const entt::entity object, const std::string& tag)
	{
		entt::entity child = mRegistry.create();
		ComponentHierarchy& hierarchy = mRegistry.emplace<ComponentHierarchy>(child);
		hierarchy.parent = object;
		hierarchy.tag = tag;
		mRegistry.get<ComponentHierarchy>(object).children.insert(child);
		return child;
	}

	entt::entity Scene::CreateObject(const std::string& tag)
	{
		entt::entity entity = mRegistry.create();
		ComponentHierarchy& hierarchy = mRegistry.emplace<ComponentHierarchy>(entity);
		hierarchy.tag = tag;
		mRootEntities.insert(entity);
		return entity;
	}

	void Scene::FinalizeLogicfFrame()
	{
		for (entt::entity e : mPendingRemoveStorage)
		{
			if (mRegistry.all_of<ComponentHierarchy>(e))
			{
				const ComponentHierarchy& hierarchy = mRegistry.get<ComponentHierarchy>(e);
				if (hierarchy.parent != entt::null)
				{
					ComponentHierarchy& parent = mRegistry.get<ComponentHierarchy>(hierarchy.parent);
					parent.children.erase(e);
				}
			}
			mRegistry.destroy(e);
		}

		if (!mDirty.empty())
		{
			for (entt::entity e : mDirty)
			{
				const bool valid = mRegistry.valid(e);
				const bool renderable = mRegistry.all_of<ComponentTransform, ComponentGeometry, ComponentMaterial>(e);
				const auto idxIt = mRenderIndex.find(e);
				const bool inList = idxIt != mRenderIndex.end();

				if (renderable && valid)
				{
					const auto& transform = mRegistry.get<ComponentTransform>(e);
					const auto& msh = mRegistry.get<ComponentGeometry>(e);
					const auto& mat = mRegistry.get<ComponentMaterial>(e);

					if (inList)
					{
						RenderItem& ri = mRenderList[idxIt->second];
						ri.world = GetObjectWorldMat(e, mRegistry);
						ri.mesh = msh;
						ri.material = mat;
					}
					else
					{
						u32 idx = mRenderList.size();
						mRenderList.emplace_back(RenderItem
							{
									.entity = e,
									.world = GetObjectWorldMat(e, mRegistry),
									.mesh = msh,
									.material = mat
							});
						mRenderIndex.emplace(e, idx);
					}
				}
				else if (inList)
				{
					const std::size_t idx = idxIt->second;
					const std::size_t last = mRenderList.size() - 1;

					if (idx != last)
					{
						mRenderList[idx] = std::move(mRenderList[last]);
						mRenderIndex[mRenderList[idx].entity] = idx;
					}
					mRenderList.pop_back();
					mRenderIndex.erase(idxIt);
				}
			}
			mDirty.clear();
		}
		
		if (!mCameraDirty.empty())
		{
			for (entt::entity e : mCameraDirty)
			{
				const bool valid = mRegistry.valid(e);
				const bool hasCamera = mRegistry.all_of<ComponentTransform, ComponentCamera>(e);
				const auto idxIt = mCameraIndex.find(e);
				const bool inList = idxIt != mCameraIndex.end();

				if (hasCamera && valid)
				{
					const auto& camera = mRegistry.get<ComponentCamera>(e);

					if (inList)
					{
						CameraItem& cameraItem = mCameras[idxIt->second];
						DirectX::XMMATRIX objectTransform = GetObjectWorldMat(e, mRegistry);
						DirectX::XMVECTOR objectPosition = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), objectTransform);
						DirectX::XMStoreFloat3(&cameraItem.position, DirectX::XMVectorAdd(objectPosition, camera.position));
						cameraItem.view = DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat3(&cameraItem.position), camera.lookDir, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
						cameraItem.fov = camera.fov;
						cameraItem.nearZ = camera.nearZ;
						cameraItem.farZ = camera.farZ;
					}
					else
					{
						u32 idx = mCameras.size();
						CameraItem& cameraItem = mCameras.emplace_back();
						DirectX::XMMATRIX objectTransform = GetObjectWorldMat(e, mRegistry);
						DirectX::XMVECTOR objectPosition = DirectX::XMVectorSet(objectTransform.r[3].m128_f32[0], objectTransform.r[3].m128_f32[1], objectTransform.r[3].m128_f32[2], 1.0f);
						DirectX::XMStoreFloat3(&cameraItem.position, DirectX::XMVectorAdd(objectPosition, camera.position));
						cameraItem.view = DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat3(&cameraItem.position), camera.lookDir, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
						cameraItem.fov = camera.fov;
						cameraItem.nearZ = camera.nearZ;
						cameraItem.farZ = camera.farZ;
						mCameraIndex.emplace(e, idx);
					}
				}
				else if (inList)
				{
					const std::size_t idx = idxIt->second;
					const std::size_t last = mCameras.size() - 1;

					if (idx != last)
					{
						mCameras[idx] = std::move(mCameras[last]);
						mRenderIndex[mCameras[idx].entity] = idx;
					}
					mCameras.pop_back();
					mCameraIndex.erase(idxIt);
				}
			}
			mCameraDirty.clear();
		}

		if (!mLightDirty.empty())
		{
			
		}
	}
}