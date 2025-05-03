#pragma once
#include "fx/shaderfx.h"
#include "content.h"
#include "entt.hpp"

namespace tiny
{
	/*struct TINYFX_API FSSamplerMaterialInstance : public fx::MaterialInstance
	{
		RenderTexture diffuseTexture;

		u64 GetMatID() override
		{
			return std::hash<std::string>()("FSSamplerMaterial");
		}

		entt::meta_any Meta() override
		{
			return entt::forward_as_meta(*this);
		}
	};

	struct TINYFX_API FSSamplerMaterial : public fx::Material
	{
		void Initialize(ID3D12Device* pDevice) override;
	};*/

	struct TINYFX_API BeamMaterialInstance : public fx::MaterialInstance<BeamMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(BeamMaterialInstance)
		struct ShaderParams
		{
			float width;
			float height;
			float time;
		};

		CBufferInline<ShaderParams> ilShaderParams;
	};

	struct TINYFX_API TextureStretcherMaterialInstance : 
		public fx::MaterialInstance<TextureStretcherMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(TextureStretcherMaterialInstance)
		RenderTexture diffuseTexture;
	};
}