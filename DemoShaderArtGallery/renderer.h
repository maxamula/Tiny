#pragma once
#include "materials.h"
#include <graphics/renderer.h>

class ShaderArtRenderer : public tiny::IRenderer
{
public:
	ShaderArtRenderer();
	~ShaderArtRenderer() = default;
	tiny::RenderTextureHandle Build(tiny::FrameGraph& mFrameGraph) override;
	void SetResolution(u32 width, u32 height);
private:
	Art1MaterialInstance mMat1;
	Art2MaterialInstance mMat2;
	Art3MaterialInstance mMat3;
	Art4MaterialInstance mMat4;
	u32 mResolutionX;
	u32 mResolutionY;
};
