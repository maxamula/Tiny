#pragma once
#include "renderctx.h"
#include "framegraph.h"
#include "engine/scene.h"

namespace tiny
{
	class TINYFX_API RenderPassBase
	{
	public:
		virtual ~RenderPassBase() = default;
		virtual void Setup(FrameGraph::Builder& builder) = 0;
		virtual void Execute(RenderContext& ctx, FrameGraphResources& res) = 0;
		void ExecuteRecordElapsed(RenderContext& ctx, FrameGraphResources& res);
		f64 EstimateCost();
	private:
		f64 mCost = 0.0;
	};

	class TINYFX_API SceneFilteredPass : public RenderPassBase
	{
	public:
		explicit SceneFilteredPass(u16 passId);
		~SceneFilteredPass() = default;
		void Execute(RenderContext& ctx, FrameGraphResources& res) override final;
		virtual void OnRenderItem(RenderContext& context, FrameGraphResources& res, const ComponentMaterial& material, const ComponentGeometry& geometry) = 0;
	protected:
		u16 mPassId;
	};
}