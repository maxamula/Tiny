#pragma once
#include "fx/shaderfx.h"
#include "content.h"
#include "entt.hpp"

namespace tiny
{
	struct TINYFX_API BeamMaterialInstance : public fx::MaterialInstance<BeamMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(BeamMaterialInstance);
		struct ShaderParams
		{
			float width;
			float height;
			float time;
		};

		ShaderParams ilShaderParams;
	};

	struct TINYFX_API TextureStretcherMaterialInstance : 
		public fx::MaterialInstance<TextureStretcherMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(TextureStretcherMaterialInstance);
		RenderTexture diffuseTexture;
	};

	struct TINYFX_API UnlitMaterialInstance : 
		public fx::MeshMaterialInstance<UnlitMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(UnlitMaterialInstance);
		Texture2D diffuseTexture;
	};
}