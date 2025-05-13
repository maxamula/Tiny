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
			.width = 1920,
			.height = 1080,
			.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				if (message == WM_CLOSE)
					PostQuitMessage(0);
				return DefWindowProc(hWnd, message, wParam, lParam);
			},
			.title = "Ocean"
		};
		tiny::Window window(desc);
		window.SetFullscreen(true);
		window.Show();
		ShaderArtRenderer renderer;
		renderer.SetResolution(1920, 1080);
		tiny::RenderView& renderView = tiny::CreateRenderView(&window, nullptr, &renderer);
		tiny::RunEngine();
	}
	tiny::Shutdown();
	return 0;
}