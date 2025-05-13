#pragma once
#include "pass.h"
#include "descriptors.h"
#include "imgui/imgui.h"

namespace tiny
{
	class ImGuiDebugPass : public tiny::RenderPassBase
	{
	public:
		ImGuiDebugPass(HWND hWnd, RenderTextureHandle targetTexture);
		~ImGuiDebugPass() override;

		void Setup(FrameGraph::Builder& builder) override;
		void Execute(RenderContext& ctx, FrameGraphResources& res) override;
		ImGuiContext* GetImGuiContext() { return mImContext; }
	private:
		RenderTextureHandle mTargetTexture;
		ImGuiContext* mImContext;
		DescriptorHandle mImguiSrv;
	};
}