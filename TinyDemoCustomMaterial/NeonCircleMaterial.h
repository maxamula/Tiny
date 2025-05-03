#pragma once
#include <meta.h>
#include <fx/shaderfx.h>
#include <fx/registry.h>
#include <content/content.h>

using namespace entt;
using namespace tiny;
using namespace tiny::fx;

struct NeonCircleMaterialInstance : public MaterialInstance
{
	struct Params { float radius; };
	CBufferInline<Params> params;
};

struct NeonCircleMaterial : public Material
{

};

META(NeonCircleMaterialInstance)
{

};

TINY_REGISTER_MATERIAL(NeonCircleMaterial, NeonCircleMaterialInstance);