#include "renderer.h"
#define TINY_REVEAL_INTERNALS
#include <graphics/pass.h>

class OceanFulscreenPass : public tiny::IRenderPass
{
public:
	OceanFulscreenPass(tiny::RenderTextureHandle target, WaterMaterialInstance* pMat)
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
		mpMaterial->ilShaderParams.data.time += 0.01f;
		ctx.BindMaterial(mpMaterial);
		ctx.cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.cmd->DrawInstanced(3, 1, 0, 0);
	}

private:
	tiny::RenderTextureHandle mTarget;
	WaterMaterialInstance* mpMaterial;
};



ShaderArtRenderer::ShaderArtRenderer()
{
	mWaterMat.SetFeatureFlags(WATER_MAT_DEFAULT_FEATURES);
	mWaterMat.ilShaderParams.data.width = 1280.0f;
	mWaterMat.ilShaderParams.data.height = 720.0f;
	mWaterMat.ilShaderParams.data.time = 0.0f;
}

tiny::RenderTextureHandle ShaderArtRenderer::Build(tiny::FrameGraph& frameGraph)
{
	tiny::RenderTexture::Desc desc
	{
		.width = 1280,
		.height = 720,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.clearValue = { DXGI_FORMAT_UNKNOWN, { 0.0f, 0.0f, 0.0f, 1.0f } }
	};

	tiny::RenderTextureHandle renderTexture = frameGraph.CreateRenderTexture(desc);
	frameGraph.AddPass<OceanFulscreenPass>(renderTexture, &mWaterMat);
	return renderTexture;
}