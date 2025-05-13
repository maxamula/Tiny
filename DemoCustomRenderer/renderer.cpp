#include "renderer.h"
#define TINY_REVEAL_INTERNALS
#include <graphics/pass.h>

class OceanFulscreenPass : public tiny::RenderPassBase
{
public:
	OceanFulscreenPass(tiny::RenderTextureHandle target, OceanMaterialInstance* pMat)
		: mTarget(target), mpMaterial(pMat)
	{}
	~OceanFulscreenPass() = default;

	void Setup(tiny::FrameGraph::Builder& builder) override
	{
		builder.Write(mTarget);
	}

	void Execute(tiny::RenderContext& ctx, tiny::FrameGraphResources& res) override
	{
		tiny::RenderTexture& targetTexture = res.GetRenderTexture(mTarget);

		D3D12_VIEWPORT viewport = { 0.0f, 0.0f, targetTexture.width, targetTexture.height, 0.0f, 1.0f };
		D3D12_RECT scissorRect = { 0, 0, targetTexture.width, targetTexture.height };
		auto rtv = targetTexture.rtv.cpu;
		ctx.cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		ctx.cmd->RSSetViewports(1, &viewport);
		ctx.cmd->RSSetScissorRects(1, &scissorRect);
		mpMaterial->ilShaderParams.time += 0.01f;
		ctx.BindMaterial(mpMaterial);
		ctx.cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.cmd->DrawInstanced(3, 1, 0, 0);
	}

private:
	tiny::RenderTextureHandle mTarget;
	OceanMaterialInstance* mpMaterial;
};



ShaderArtRenderer::ShaderArtRenderer()
{
	mMat1.SetFeatureFlags(WATER_MAT_DEFAULT_FEATURES);
	mMat1.ilShaderParams.time = 0.0f;
}

tiny::RenderTextureHandle ShaderArtRenderer::Build(tiny::FrameGraph& frameGraph)
{
	tiny::RenderTexture::Desc desc
	{
		.width = mResolutionX,
		.height = mResolutionY,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.clearValue = { DXGI_FORMAT_UNKNOWN, { 0.0f, 0.0f, 0.0f, 1.0f } }
	};

	tiny::RenderTextureHandle renderTexture = frameGraph.CreateRenderTexture(desc);
	frameGraph.AddPass<OceanFulscreenPass>(renderTexture, &mMat1);
	return renderTexture;
}

void ShaderArtRenderer::SetResolution(u32 width, u32 height)
{
	mResolutionX = width;
	mResolutionY = height;
	mMat1.ilShaderParams.width = static_cast<f32>(width);
	mMat1.ilShaderParams.height = static_cast<f32>(height);
}