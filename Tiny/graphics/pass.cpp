#include "pass.h"


namespace tiny
{
	SceneFilteredPass::SceneFilteredPass(u16 passId)
		: mPassId(passId)
	{}

	void SceneFilteredPass::Execute(RenderContext& ctx, FrameGraphResources& res)
	{
		ctx.renderItems->view<ComponentMaterial, ComponentGeometry>().each([&](entt::entity entity, ComponentMaterial& mat, ComponentGeometry& geo)
		{ 
				auto it = mat.passes.find(mPassId);
				if (it != mat.passes.end())
					OnRenderItem(ctx, res, mat, geo);
		});
	}
}