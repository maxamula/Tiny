#pragma once
#include "content.h"

namespace tiny
{
	RenderTexture& GetCachedFrameGraphResource(const RenderTexture::Desc& d);
	void ReleaseCachedFrameGraphResource(RenderTexture& t);
	void InvalidateCachedFrameGraphResources();
}