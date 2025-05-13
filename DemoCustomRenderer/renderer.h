#pragma once
#include "watermat.h"
#include <graphics/renderer.h>

class ShaderArtRenderer : public tiny::IRenderer
{
public:
	ShaderArtRenderer();
	~ShaderArtRenderer() = default;
	tiny::RenderTextureHandle Build(tiny::FrameGraph& mFrameGraph) override;
	void SetResolution(u32 width, u32 height);
private:
	OceanMaterialInstance mMat1;
	u32 mResolutionX;
	u32 mResolutionY;
};