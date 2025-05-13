#include "renderer.h"
#include <graphics/pass.h>

std::atomic<u32> gSelectedArt = 0;

class ShaderArtFullscreenPass : public tiny::RenderPassBase
{
public:
	ShaderArtFullscreenPass(tiny::RenderTextureHandle target, Art1MaterialInstance* pMat1, Art2MaterialInstance* pMat2, Art3MaterialInstance* pMat3, Art4MaterialInstance* pMat4)
		: mTarget(target), mpMaterial(pMat1), mpMaterial2(pMat2), mpMaterial3(pMat3), mpMaterial4(pMat4)
	{}
	~ShaderArtFullscreenPass() = default;

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
		f32 color[4] = { 0.01f, 0.01f, 0.01f, 1.0f };
		ctx.cmd->ClearRenderTargetView(rtv, color, 0, nullptr);
		ctx.cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		ctx.cmd->RSSetViewports(1, &viewport);
		ctx.cmd->RSSetScissorRects(1, &scissorRect);
		mpMaterial->ilShaderParams.time += 0.01f;
		switch (gSelectedArt)
		{
		case 0:
			mpMaterial->ilShaderParams.time += 0.01f;
			ctx.BindMaterial(mpMaterial);
			break;
		case 1:
			mpMaterial2->ilShaderParams.time += 0.01f;
			ctx.BindMaterial(mpMaterial2);
			break;
		case 2:
			mpMaterial3->ilShaderParams.time += 0.01f;
			ctx.BindMaterial(mpMaterial3);
			break;
		case 3:
			mpMaterial4->ilShaderParams.time += 0.01f;
			ctx.BindMaterial(mpMaterial4);
			break;
		default:
			mpMaterial->ilShaderParams.time += 0.01f;
			ctx.BindMaterial(mpMaterial);
			break;
		}
		ctx.cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.cmd->DrawInstanced(3, 1, 0, 0);
	}

private:
	tiny::RenderTextureHandle mTarget;
	Art1MaterialInstance* mpMaterial;
	Art2MaterialInstance* mpMaterial2;
	Art3MaterialInstance* mpMaterial3;
	Art4MaterialInstance* mpMaterial4;
};



ShaderArtRenderer::ShaderArtRenderer()
{
	mMat1.SetFeatureFlags(MAT_DEFAULT_FEATURES);
	mMat1.ilShaderParams.time = 0.0f;

	mMat2.SetFeatureFlags(MAT_DEFAULT_FEATURES);
	mMat2.ilShaderParams.time = 0.0f;

	mMat3.SetFeatureFlags(MAT_DEFAULT_FEATURES);
	mMat3.ilShaderParams.time = 0.0f;

	mMat4.SetFeatureFlags(MAT_DEFAULT_FEATURES);
	mMat4.ilShaderParams.time = 0.0f;
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
	frameGraph.AddPass<ShaderArtFullscreenPass>(renderTexture, &mMat1, &mMat2, &mMat3, &mMat4);
	return renderTexture;
}

void ShaderArtRenderer::SetResolution(u32 width, u32 height)
{
	mResolutionX = width;
	mResolutionY = height;
	mMat1.ilShaderParams.width = static_cast<f32>(width);
	mMat1.ilShaderParams.height = static_cast<f32>(height);
}