#include <Windows.h>
#include <engine/application.h>
#include "rendering/rendererhost.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<tiny::RendererHost> gRendererHost;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    tiny::Initialize();
    {
		AllocConsole();
		FILE* pFile;
		freopen_s(&pFile, "CONOUT$", "w", stdout);
		tiny::WindowDesc desc
		{
			.posX = 100,
			.posY = 100,
			.width = 1280,
			.height = 720,
			.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				// process imgui
				if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
					return true;

				switch (message)
				{
				case WM_CLOSE:
					PostQuitMessage(0);
					break;
				case WM_SIZE:
					break;
				default:
					break;
				}
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		};
        tiny::Window w(desc);
		gRendererHost = std::make_unique<tiny::RendererHost>(w.hWnd);
		tiny::CreateRenderView(&w, gRendererHost.get());
        tiny::RunEngine();
		gRendererHost.reset();
    }
    tiny::Shutdown();
}