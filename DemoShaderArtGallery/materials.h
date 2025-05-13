#pragma once
#include "fx/shaderfx.h"
#include "content/content.h"

#define MAT_DEFAULT_FEATURES 0

using namespace entt;

struct Art1MaterialInstance : public tiny::fx::MaterialInstance<Art1MaterialInstance>
{
	TINY_DECLARE_MAT_ID(Art1MaterialInstance);
	struct ShaderParams
	{
		f32 width;
		f32 height;
		f32 time;
	};
	ShaderParams ilShaderParams;
};

struct Art2MaterialInstance : public tiny::fx::MaterialInstance<Art2MaterialInstance>
{
	TINY_DECLARE_MAT_ID(Art2MaterialInstance);
	struct ShaderParams
	{
		f32 width;
		f32 height;
		f32 time;
	};
	ShaderParams ilShaderParams;
};

struct Art3MaterialInstance : public tiny::fx::MaterialInstance<Art3MaterialInstance>
{
	TINY_DECLARE_MAT_ID(Art3MaterialInstance);
	struct ShaderParams
	{
		f32 width;
		f32 height;
		f32 time;
	};
	ShaderParams ilShaderParams;
};

struct Art4MaterialInstance : public tiny::fx::MaterialInstance<Art4MaterialInstance>
{
	TINY_DECLARE_MAT_ID(Art4MaterialInstance);
	struct ShaderParams
	{
		f32 width;
		f32 height;
		f32 time;
	};
	ShaderParams ilShaderParams;
};