#pragma once
#include "fx/shaderfx.h"

using namespace entt;

namespace tiny
{
	struct GridMaterialInstance : 
		public fx::MaterialInstance<GridMaterialInstance>
	{
		TINY_DECLARE_MAT_ID(IMaterialInstance);
	};
}