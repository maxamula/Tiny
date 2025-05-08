#include "renderer.h"
#include "pass.h"

#include "content/stdmat.h"

namespace tiny
{
	class OpaquePass : public SceneFilteredPass
	{
	public:
		explicit OpaquePass(u16 passId, DepthTextureHandle depthTexture, RenderTextureHandle renderTexture) : SceneFilteredPass(passId),
			mDepthTexture(depthTexture), mRenderTexture(renderTexture)
		{ }

		void Setup(FrameGraph::Builder& builder) override
		{
			builder.Write(mRenderTexture);
			builder.Write(mDepthTexture);
		}

		void OnRenderItem(RenderContext& context, FrameGraphResources& res, const ComponentMaterial& material, const ComponentGeometry& geometry) override
		{
			const auto& mat = material.passes.find(mPassId);
			if (mat == material.passes.end())
				return;

			RenderTexture& rt = res.GetRenderTexture(mRenderTexture);
			DepthTexture& dt = res.GetDepthTexture(mDepthTexture);

			context.cmd->ClearDepthStencilView(dt.dsv.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, rt.width, rt.height, 0.0f, 1.0f };
			D3D12_RECT scissorRect = { 0, 0, rt.width, rt.height };
			auto rtv = rt.rtv.cpu;
			auto dsv = dt.dsv.cpu;
			context.cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
			context.cmd->RSSetViewports(1, &viewport);
			context.cmd->RSSetScissorRects(1, &scissorRect);
			context.BindMaterial(mat->second.get(), geometry.inputAttributes);
			context.BindMesh(geometry);
			context.cmd->DrawIndexedInstanced(geometry.numIndices, 1, 0, 0, 0);
		}

	private:
		DepthTextureHandle mDepthTexture;
		RenderTextureHandle mRenderTexture;
	};

	RenderTextureHandle TinyForwardRenderer::Build(FrameGraph& frameGraph)
	{
		RenderTexture::Desc desc
		{
			.width = 1280,
			.height = 720,
			.format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.clearValue = { DXGI_FORMAT_UNKNOWN, { 0.0f, 0.0f, 0.0f, 1.0f } }
		};

		RenderTextureHandle renderTexture = frameGraph.CreateRenderTexture(desc);

		DepthTexture::Desc depthDesc
		{
			.width = 1280,
			.height = 720,
		};
		DepthTextureHandle depthTexture = frameGraph.CreateDepthTexture(depthDesc);

		frameGraph.AddPass<OpaquePass>(fx::MeshTechnique::RenderPassId_Opaque, depthTexture, renderTexture);

		return renderTexture;
	}
}