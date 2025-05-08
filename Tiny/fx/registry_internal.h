#pragma once
#include "shaderfx.h"
#include "registry.h"

namespace tiny::fx
{
	struct MaterialInfo
	{
		fnMaterialInitializer	initializer;
		fnMaterialFactory		factory;
		Material				material;
	};

	struct MeshMaterialInfo
	{
		fnMeshMaterialInitializer	initializer;
		fnMeshMaterialFactory		factory;
		MeshMaterial				material;
	};

	std::unordered_map<u64, MaterialInfo>& GetMaterialsRegistry();
	std::unordered_map<u64, MeshMaterialInfo>& GetMeshMaterialsRegistry();
}