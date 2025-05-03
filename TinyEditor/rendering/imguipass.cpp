#include "imguipass.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

namespace tiny
{
	void RenderUI()
	{
		// Full-screen parent window for docking
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// Create dockspace
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		// Initial layout setup (only run once)
		static bool first_time_setup = true;
		if (first_time_setup) {
			first_time_setup = false;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGuiID main_dock_id = ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags);

			ImGuiID center = main_dock_id;
			ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, nullptr, &center);
			ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
			ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.4f, nullptr, &center);

			ImGuiID left_top = left;
			ImGuiID left_bottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.5f, nullptr, &left_top);

			ImGui::DockBuilderDockWindow("Scene Hierarchy", left_top);
			ImGui::DockBuilderDockWindow("Scene Debugger", left_bottom);
			ImGui::DockBuilderDockWindow("Scene Viewport", center);
			ImGui::DockBuilderDockWindow("Entity Components", right);
			ImGui::DockBuilderDockWindow("Asset Browser", bottom);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGui::Begin("Scene Hierarchy");
		ImGui::End();
		ImGui::Begin("Scene Debugger");
		ImGui::End();
		ImGui::Begin("Scene Viewport");
		ImGui::End();
		ImGui::Begin("Entity Components");
		ImGui::End();
		ImGui::Begin("Asset Browser");
		ImGui::End();



		ImGui::End();
	}

	void ImGuiPass::Execute(RenderContext& ctx, FrameGraphResources& res)
	{
		RenderTexture& tex = res.GetRenderTexture(outputTexture);
		auto rtv = tex.rtv.cpu;

		ctx.cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		//ImGui::ShowDemoWindow();
		RenderUI();
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ctx.cmd);
	}
}