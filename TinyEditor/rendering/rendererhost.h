#pragma once
#include <memory>
#include <graphics/renderer.h>
#include <graphics/descriptors.h>

#include "imgui/imgui.h"

namespace tiny
{
	class RendererHost : public TinyForwardRenderer
	{
	public:
		RendererHost(HWND hwnd);
		~RendererHost();

		RenderTextureHandle Build(FrameGraph& mFrameGraph) override;
	private:
		std::unique_ptr<IRenderer> mHostedRederer;
		ImGuiContext* mImGuiContext;
		DescriptorHandle srvImgui;
	};
}