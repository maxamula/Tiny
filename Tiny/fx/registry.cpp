#include "registry.h"
#include "registry_internal.h"
#include <unordered_map>

namespace tiny::fx
{
	std::unordered_map<u64, MaterialInfo>& GetMaterialsRegistry()
	{
		static std::unordered_map<u64, MaterialInfo> materials;
		return materials;
	}
	std::unordered_map<u64, MeshMaterialInfo>& GetMeshMaterialsRegistry()
	{
		static std::unordered_map<u64, MeshMaterialInfo> meshMaterials;
		return meshMaterials;
	}

	u8 RegisterMaterial(u64 instanceId, fnMaterialInitializer initializer, fnMaterialFactory factory)
	{
		auto& materials = GetMaterialsRegistry();
		if (materials.find(instanceId) != materials.end())
			return 1;
		materials[instanceId] = MaterialInfo
		{ 
			.initializer = initializer,
			.factory = factory,
			.material = {}
		};
		return 0;
	}

	u8 RegisterMeshMaterial(u64 instanceId, fnMeshMaterialInitializer initializer, fnMeshMaterialFactory factory)
	{
		auto& meshMaterials = GetMeshMaterialsRegistry();
		if (meshMaterials.find(instanceId) != meshMaterials.end())
			return 1;
		meshMaterials[instanceId] = MeshMaterialInfo
		{
			.initializer = initializer,
			.factory = factory,
			.material = {}
		};
		return 0;
	}
}