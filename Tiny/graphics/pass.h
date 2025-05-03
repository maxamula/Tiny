#pragma once
#include "renderctx.h"
#include "engine/scene.h"

namespace tiny
{
	class RenderContext;
	class FrameGraphResources;

	class TINYFX_API IRenderPass
	{
	public:
		virtual ~IRenderPass() = default;
		virtual void Execute(RenderContext& ctx, FrameGraphResources& res) = 0;
	};

	class TINYFX_API SceneFilteredPass : public IRenderPass
	{
	public:
		explicit SceneFilteredPass(u16 passId);
		~SceneFilteredPass() = default;
		void Execute(RenderContext& ctx, FrameGraphResources& res) override final;
		virtual void OnRenderItem(RenderContext& context, FrameGraphResources& res, const ComponentRenderItem& renderItem) = 0;
	protected:
		u16 mPassId;
	};
}