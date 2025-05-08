#pragma once
#include "fx/shaderfx.h"
#include "content/content.h"

#define WATER_MAT_DEFAULT_FEATURES 0

using namespace entt;

struct WaterMaterialInstance : public tiny::fx::MaterialInstance<WaterMaterialInstance>
{
	TINY_DECLARE_MAT_ID(WaterMaterialInstance);
	
	struct ShaderParams
	{
		float width;
		float height;
		float time;
	};

	tiny::CBufferInline<ShaderParams> ilShaderParams;
};