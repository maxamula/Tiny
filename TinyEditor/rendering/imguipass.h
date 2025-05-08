#pragma once
#include "imgui/imgui.h"
#include <graphics/pass.h>
#include <graphics/renderer.h>

namespace tiny
{
	class ImGuiPass : public IRenderPass
	{
	public:
		ImGuiPass(RenderTextureHandle outputTexture, RenderTextureHandle hostedTexture);
		~ImGuiPass() = default;

		void Setup(FrameGraph::Builder& builder) override;
		void Execute(RenderContext& ctx, FrameGraphResources& res) override;

	private:
		const RenderTextureHandle mOutputTexture;
		const RenderTextureHandle mHostedTexture;
	};
}

