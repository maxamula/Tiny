#pragma once
#include "framegraph.h"

namespace tiny
{
	class TINYFX_API IRenderer
	{
	public:
		virtual ~IRenderer() = default;
		virtual RenderTextureHandle Build(FrameGraph& mFrameGraph) = 0;
	};

	class TINYFX_API TinyForwardRenderer : public IRenderer
	{
	public:
		virtual ~TinyForwardRenderer() override = default;
		virtual RenderTextureHandle Build(FrameGraph& mFrameGraph) override;
		void SetResolution(u32 width, u32 height);
	protected:
		u32 mResolutionX;
		u32 mResolutionY;
	};

	class TinyDeferredRenderer : public IRenderer
	{
	public:
		virtual ~TinyDeferredRenderer() = default;
		virtual RenderTextureHandle Build(FrameGraph& mFrameGraph) override;
	};
}