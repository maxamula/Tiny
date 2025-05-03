#include "pass.h"


namespace tiny
{
	SceneFilteredPass::SceneFilteredPass(u16 passId)
		: mPassId(passId)
	{}

	void SceneFilteredPass::Execute(RenderContext& ctx, FrameGraphResources& res)
	{
		ctx.renderItems->view<ComponentRenderItem>().each([&](entt::entity entity, ComponentRenderItem& item)
		{ 
				auto it = item.technique.passes.find(mPassId);
				if (it != item.technique.passes.end())
					OnRenderItem(ctx, res, item);
		});
	}
}