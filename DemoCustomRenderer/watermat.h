#pragma once
#include "fx/shaderfx.h"
#include "content/content.h"

#define WATER_MAT_DEFAULT_FEATURES 0

using namespace entt;

struct OceanMaterialInstance : public tiny::fx::MaterialInstance<OceanMaterialInstance>
{
	TINY_DECLARE_MAT_ID(OceanMaterialInstance);
	
	struct ShaderParams
	{
		f32 width;
		f32 height;
		f32 time;
	};

	ShaderParams ilShaderParams;
};