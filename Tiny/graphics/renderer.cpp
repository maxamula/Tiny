#include "renderer.h"
#include "pass.h"

#include "content/stdmat.h"

namespace tiny
{
	class BeamPass : public IRenderPass
	{
	public:
		void Execute(RenderContext& ctx, FrameGraphResources& res) override
		{
			RenderTexture& output = res.GetRenderTexture(outputTexture);

			auto rtv = output.rtv.cpu;
			ctx.cmd->OMSetRenderTargets(1, &output.rtv.cpu, FALSE, nullptr);

			BeamMaterialInstance mat;
			mat.SetFeatureFlags(0);
			

		}

		RenderTextureHandle outputTexture;
	};

	RenderTextureHandle TinyForwardRenderer::Build(FrameGraph& mFrameGraph)
	{

		return {};
	}
}