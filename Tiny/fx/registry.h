#pragma once
#include <memory>
#include "shaderfx.h"

using namespace entt;

#define TINY_REGISTER_MATERIAL(INITIALIZER, INSTANCE_TYPE) void INITIALIZER(ID3D12Device*, tiny::fx::Material&); struct INSTANCE_TYPE; namespace { const u8 _reg##INSTANCE_TYPE{tiny::fx::RegisterMaterial(#INSTANCE_TYPE##_hs, &INITIALIZER, &tiny::fx::MaterialInstanceFactory<INSTANCE_TYPE>)};}
#define TINY_REGISTER_MESH_MATERIAL(INITIALIZER, INSTANCE_TYPE) void INITIALIZER(ID3D12Device*, tiny::fx::MeshMaterial&); struct INSTANCE_TYPE; namespace { const u8 _reg##INSTANCE_TYPE{tiny::fx::RegisterMeshMaterial(#INSTANCE_TYPE##_hs, &INITIALIZER, &tiny::fx::MeshMaterialInstanceFactory<INSTANCE_TYPE>)};}

namespace tiny::fx
{
	using fnMaterialInitializer = void(*)(ID3D12Device* pDevice, Material& mat);
	using fnMeshMaterialInitializer = void(*)(ID3D12Device* pDevice, MeshMaterial& mat);
	using fnMaterialFactory = std::shared_ptr<fx::IMaterialInstance>(*)();
	using fnMeshMaterialFactory = std::shared_ptr<fx::IMeshMaterialInstance>(*)();

	TINYFX_API u8 RegisterMaterial(u64 instanceId, fnMaterialInitializer initializer, fnMaterialFactory factory);
	TINYFX_API u8 RegisterMeshMaterial(u64 instanceId, fnMeshMaterialInitializer initializer, fnMeshMaterialFactory factory);

	template <typename T>
	std::shared_ptr<fx::IMaterialInstance> MaterialInstanceFactory()
	{
		return std::make_shared<T>();
	}

	template <typename T>
	std::shared_ptr<fx::IMeshMaterialInstance> MeshMaterialInstanceFactory()
	{
		return std::make_shared<T>();
	}
}