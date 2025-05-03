#pragma once
#include "imgui/imgui.h"
#include <graphics/pass.h>
#include <graphics/renderer.h>

namespace tiny
{
	class ImGuiPass : public IRenderPass
	{
	public:
		ImGuiPass() = default;
		~ImGuiPass() = default;

		void Execute(RenderContext& ctx, FrameGraphResources& res) override;

		RenderTextureHandle viewportTexture;
		RenderTextureHandle outputTexture;
	};
}

