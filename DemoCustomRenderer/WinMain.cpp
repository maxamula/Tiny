#include <Windows.h>
#include <engine/application.h>
#include "renderer.h"

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	tiny::Initialize();
	{
		tiny::WindowDesc desc
		{
			.posX = 100,
			.posY = 100,
			.width = 1280,
			.height = 720,
			.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				if (message == WM_CLOSE)
					PostQuitMessage(0);
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		};
		tiny::Window window(desc);
		ShaderArtRenderer renderer;
		tiny::RenderView* renderView = tiny::CreateRenderView(&window, &renderer);
		tiny::RunEngine();
	}
	tiny::Shutdown();
	return 0;
}