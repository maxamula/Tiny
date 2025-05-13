#include "gfxdebug.h"
#include "gfxglobal.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

namespace tiny
{
	extern "C++" D3DContext gContext;

	ImGuiDebugPass::ImGuiDebugPass(HWND hWnd, RenderTextureHandle targetTexture)
		: mTargetTexture(targetTexture), mImContext(nullptr), mImguiSrv(GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor())
	{
		mImContext = ImGui::CreateContext();
		ImGui::StyleColorsDark(mImContext);
		ImGuiIO& io = ImGui::GetIO(mImContext); (void)io;
		ImGui_ImplWin32_Init(mImContext, hWnd);
		ImGui_ImplDX12_Init(mImContext, gDevice, gContext.framesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM,
			*GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
			mImguiSrv.cpu, mImguiSrv.gpu);
	}

	ImGuiDebugPass::~ImGuiDebugPass()
	{
		if (mImContext)
		{
			ImGui_ImplDX12_Shutdown(mImContext);
			ImGui_ImplWin32_Shutdown(mImContext);
			ImGui::DestroyContext(mImContext);
		}
	}

	void ImGuiDebugPass::Setup(FrameGraph::Builder& builder)
	{
		builder.Write(mTargetTexture);
	}

	void ImGuiDebugPass::Execute(RenderContext& ctx, FrameGraphResources& res)
	{
		RenderTexture targetTexture = res.GetRenderTexture(mTargetTexture);
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = targetTexture.rtv.cpu;

		D3D12_VIEWPORT viewport = { 0.0f, 0.0f, targetTexture.width, targetTexture.height, 0.0f, 1.0f };
		D3D12_RECT scissorRect = { 0, 0, targetTexture.width, targetTexture.height };
		ctx.cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		ctx.cmd->RSSetViewports(1, &viewport);
		ctx.cmd->RSSetScissorRects(1, &scissorRect);

		ImGuiIO& io = ImGui::GetIO(mImContext);
		io.DisplaySize = ImVec2(static_cast<f32>(targetTexture.width), static_cast<f32>(targetTexture.height));
		ImGuiViewport* imguiViewport = ImGui::GetMainViewport(mImContext);
		ImVec2 size = imguiViewport->Size;
		io.DisplayFramebufferScale = ImVec2(
			static_cast<f32>(targetTexture.width) / (float)size.x,
			static_cast<f32>(targetTexture.height) / (float)size.y
		);

		ImGui_ImplDX12_NewFrame(mImContext);
		ImGui_ImplWin32_NewFrame(mImContext);
		ImGui::NewFrame(mImContext);
		ImGui::ShowDemoWindow(mImContext);
		ImGui::Render(mImContext);
		ImGui_ImplDX12_RenderDrawData(mImContext, ImGui::GetDrawData(mImContext), ctx.cmd);
	}
}