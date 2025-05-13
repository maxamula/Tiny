#include <Windows.h>
#include <engine/application.h>
#include "renderer.h"

extern "C++" std::atomic<u32> gSelectedArt;

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
				if (message == WM_KEYDOWN)
				{
					if (wParam == '1')
					{
						gSelectedArt = 0;
					}
					else if (wParam == '2')
					{
						gSelectedArt = 1;
					}
					else if (wParam == '3')
					{
						gSelectedArt = 2;
					}
					else if (wParam == '4')
					{
						gSelectedArt = 3;
					}
				}
				return DefWindowProc(hWnd, message, wParam, lParam);
			},
			.title = "Shader Art Gallery",
		};
		tiny::Window window(desc);
		window.Show();
		ShaderArtRenderer renderer;
		renderer.SetResolution(1280, 720);
		tiny::RenderView& renderView = tiny::CreateRenderView(&window, nullptr, &renderer);
		tiny::RunEngine();
	}
	tiny::Shutdown();
	return 0;
}