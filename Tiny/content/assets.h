#pragma once
#include <vector>
#include <variant>
#include "fx/shaderfx.h"
#include "content.h"

namespace tiny
{
	struct SerializedCBuffer;
	struct SerializedTexture;
	struct SerializedMaterialProperty;
	struct SerializedMaterial;

	struct SerializedCBuffer
	{
		std::vector<u8> data;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(data);
		}
	};

	struct SerializedTexture
	{
		u16 width;
		u16 height;
		u16 depth;
		u16 mipLevels;
		std::vector<u8> data;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(width, height, depth, mipLevels, data);
		}
	};

	struct SerializedMaterialProperty
	{
		u64 propertyID;
		std::variant<SerializedCBuffer, SerializedTexture> value;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(propertyID, value);
		}
	};

	struct SerializedMesh
	{
		u32 inputAttributes;
		std::vector<f32> vertices;
		std::vector<u32> indices;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(inputAttributes, vertices, indices);
		}
	};

	struct SerializedMaterial
	{
		u64 matID;
		u64 features;
		std::vector<SerializedMaterialProperty> properties;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(matID, features, properties);
		}
	};

	struct SerializedTechnique
	{
		std::unordered_map<u64, SerializedMaterial> materials;	// render pass ID -> material

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(materials);
		}
	};


	TINYFX_API Mesh AssetCreateMesh(const SerializedMesh& mesh);
	TINYFX_API Texture2D AssetCreateTexture(const SerializedTexture& texture);
	TINYFX_API std::shared_ptr<fx::IMeshMaterialInstance> AssetCreateMeshMaterial(const SerializedMaterial& material);
	TINYFX_API fx::MeshTechnique AssetCreateTechnique(const SerializedTechnique& texture);

	TINYFX_API void AssetSaveMaterial(const std::string& path, const SerializedMaterial& material);
	TINYFX_API void AssetSaveTechnique(const std::string& path, const SerializedTechnique& technique);
	TINYFX_API void AssetSaveMesh(const std::string& path, const SerializedMesh& mesh);
	TINYFX_API SerializedMesh AssetLoadMesh(const std::string& path);
	TINYFX_API SerializedTechnique AssetLoadTechnique(const std::string& path);
}