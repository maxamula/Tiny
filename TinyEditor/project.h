#pragma once
#include <engine/scene.h>
#include "rendering/rendererhost.h"

namespace tiny
{
	struct Project
	{
		Project();

		Scene scene;
		RendererHost rendererHost;
		//RenderView renderView;
	};
}