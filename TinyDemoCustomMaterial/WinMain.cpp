#include <windows.h>
#include <engine/application.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    tiny::Initialize();
    {
        tiny::WindowDesc desc
        {
            .posX = SW_SHOWDEFAULT,
            .posY = SW_SHOWDEFAULT,
            .width = 1280,
            .height = 720,
            .proc = nullptr
        };
		tiny::Window window(desc);

		tiny::RunEngine();
    }
    tiny::Shutdown();
}