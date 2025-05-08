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
	protected:

	};

	class TinyDeferredRenderer : public IRenderer
	{
	public:
		virtual ~TinyDeferredRenderer() = default;
		virtual RenderTextureHandle Build(FrameGraph& mFrameGraph) override;
	};
}