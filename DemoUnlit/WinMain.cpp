#include <Windows.h>
#define TINY_REVEAL_INTERNALS
#include <engine/application.h>
#include <content/assets.h>

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
		tiny::TinyForwardRenderer renderer;
		tiny::RenderView* renderView = tiny::CreateRenderView(&window, &renderer);

		tiny::Scene& scene = tiny::GetScene();
		auto object = scene.CreateObject("test object");

		tiny::SerializedTechnique serializedTechnique = tiny::AssetLoadTechnique("unlit.technique");
		tiny::fx::MeshTechnique unlitTechnique = tiny::AssetCreateTechnique(serializedTechnique);

		tiny::SerializedMesh serializedMesh_ = tiny::AssetLoadMesh("mesh.asset");
		tiny::Mesh mesh_ = tiny::AssetCreateMesh(serializedMesh_);

		scene.GetRegistry().emplace<tiny::ComponentGeometry>(object, mesh_);
		scene.GetRegistry().emplace<tiny::ComponentMaterial>(object, unlitTechnique);

		tiny::RunEngine();
	}
	tiny::Shutdown();
	return 0;
}