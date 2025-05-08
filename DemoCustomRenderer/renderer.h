#pragma once
#include "watermat.h"
#include <graphics/renderer.h>

class ShaderArtRenderer : public tiny::IRenderer
{
public:
	ShaderArtRenderer();
	~ShaderArtRenderer() = default;
	tiny::RenderTextureHandle Build(tiny::FrameGraph& mFrameGraph) override;
private:
	WaterMaterialInstance mWaterMat;
};