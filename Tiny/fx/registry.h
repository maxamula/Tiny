#pragma once
#include <memory>
#include "shaderfx.h"

using namespace entt;

#define TINY_REGISTER_MATERIAL(INITIALIZER, INSTANCE_TYPE) void INITIALIZER(ID3D12Device*, Material&); class INSTANCE_TYPE; namespace { const u8 _reg##INSTANCE_TYPE{tiny::fx::RegisterMaterial(#INSTANCE_TYPE##_hs, &INITIALIZER)};}
#define TINY_REGISTER_MESH_MATERIAL(INITIALIZER, INSTANCE_TYPE) void INITIALIZER(ID3D12Device*, MeshMaterial&); class INSTANCE_TYPE; namespace { const u8 _reg##INSTANCE_TYPE{tiny::fx::RegisterMeshMaterial(#INSTANCE_TYPE##_hs, &INITIALIZER)};}

namespace tiny::fx
{
	using fnMaterialInitializer = void(*)(ID3D12Device* pDevice, Material& mat);
	using fnMeshMaterialInitializer = void(*)(ID3D12Device* pDevice, MeshMaterial& mat);

	TINYFX_API u8 RegisterMaterial(u64 instanceId, fnMaterialInitializer initializer);
	TINYFX_API u8 RegisterMeshMaterial(u64 instanceId, fnMeshMaterialInitializer initializer);
}