#include "pass.h"
#include "engine/application.h"
#include <chrono>


namespace tiny
{

	void RenderPassBase::ExecuteRecordElapsed(RenderContext& ctx, FrameGraphResources& res)
	{
		auto start = std::chrono::high_resolution_clock::now();
		Execute(ctx, res);
		auto end = std::chrono::high_resolution_clock::now();
		mCost = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	}

	f64 RenderPassBase::EstimateCost()
	{
		return mCost;
	}

	SceneFilteredPass::SceneFilteredPass(u16 passId)
		: mPassId(passId)
	{}

	void SceneFilteredPass::Execute(RenderContext& ctx, FrameGraphResources& res)
	{
		for (const Scene::RenderItem& renderItem : ctx.renderView.scene->mRenderList)
		{
			auto it = renderItem.material.passes.find(mPassId);
			if (it != renderItem.material.passes.end())
			{
				ctx.SetWorld(renderItem.world);
				OnRenderItem(ctx, res, renderItem.material, renderItem.mesh);
			}
		}
	}
}