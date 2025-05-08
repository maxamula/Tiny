#include "rendererhost.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include "gui/gui.h"

#include <graphics/gfxglobal.h>
#include "imguipass.h"

namespace tiny
{
	RendererHost::RendererHost(HWND hwnd)
		: mHostedRederer(std::make_unique<TinyForwardRenderer>())
	{
		IMGUI_CHECKVERSION();
		mImGuiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(mImGuiContext);
		StyleColorsTiny();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
		io.ConfigDockingWithShift = true;
		io.ConfigWindowsResizeFromEdges = true;

		auto& heap = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		srvImgui = heap.AllocateDescriptor();

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(gDevice, 3, DXGI_FORMAT_R8G8B8A8_UNORM, *heap, srvImgui.cpu, srvImgui.gpu);
	}

	RendererHost::~RendererHost()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(mImGuiContext);
	}

	RenderTextureHandle RendererHost::Build(FrameGraph& mFrameGraph)
	{
		ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
		ImVec2 size = imguiViewport->Size;

		RenderTexture::Desc desc
		{
			.format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.clearValue = { DXGI_FORMAT_UNKNOWN, { 0.0f, 0.0f, 0.0f, 1.0f } }
		};
		if (size.x > 0 && size.y > 0)
		{
			desc.width = static_cast<u32>(size.x);
			desc.height = static_cast<u32>(size.y);
		}
		else
		{
			desc.width = 1264;
			desc.height = 681;
		}

		RenderTextureHandle hostedTexture = mHostedRederer->Build(mFrameGraph);

		RenderTextureHandle outputTexture = mFrameGraph.CreateRenderTexture(desc);
		mFrameGraph.AddPass<ImGuiPass>(outputTexture, hostedTexture);
		return outputTexture;
	}
}