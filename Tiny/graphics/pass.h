#pragma once
#include "renderctx.h"
#include "framegraph.h"
#include "engine/scene.h"


namespace tiny
{
	class TINYFX_API IRenderPass
	{
	public:
		virtual ~IRenderPass() = default;
		virtual void Setup(FrameGraph::Builder& builder) = 0;
		virtual void Execute(RenderContext& ctx, FrameGraphResources& res) = 0;
		f64 EstimateCost() { return 450.0; }
	};

	class TINYFX_API SceneFilteredPass : public IRenderPass
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