#include <Tiny/fx/registry.h>
#include <Tiny/fx/shaderfx.h>
#include <Tiny/content/stdmat.h>
#include <Tiny/content/assets.h>
#include <Tiny/engine/application.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <string>
#include <vector>
#include <variant>
#include <any>

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/archives/json.hpp" // or binary/xml

#include "graphics/pass.h"
#include "graphics/renderer.h"

#include <fstream>
#include "Tiny/common.h"

#include "graphics/framegraph.h"

#include <iostream>

void Test2(tiny::fx::IMeshMaterialInstance* i)
{

	entt::meta_any meta = i->Meta();
	entt::meta_type type = meta.type();

	std::cout << "Type: " << type.info().name() << "\n";
	for (const auto& field : type.data())
	{
		std::cout << "Field: " << field.second.type().info().name() << "\n";

		entt::meta_any value = field.second.get(meta);
		//std::cout << "Value: " << value.type().base().begin()->second.info().name() << "\n";
		if (auto icbuffer = value.try_cast<tiny::ICBufferCPU>())
		{
			std::cout << "resource ptr: " << icbuffer->resource << std::endl;
		}
	}
}

using namespace tiny::fx;

class FakeWritterPass : public tiny::IRenderPass
{
public:
	FakeWritterPass() = default;
	~FakeWritterPass() = default;

	void Execute(tiny::RenderContext& ctx, const tiny::FrameGraphResources& res) override
	{
		std::cout << "Faking writing pass\n";
	}
};

class TinyEditorGraphBuilder : public tiny::IRenderer
{
public:
	tiny::RenderTextureHandle Build(tiny::FrameGraph& frameGraph) override
	{
		tiny::RenderTexture::Desc desc;
		desc.width = 512;
		desc.height = 512;
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.clearValue.Color[0] = 0.3f;
		desc.clearValue.Color[1] = 0.1f;
		desc.clearValue.Color[2] = 0.1f;
		desc.clearValue.Color[3] = 1.0f;
		tiny::RenderTextureHandle finalTex = frameGraph.CreateRenderTexture(desc);

		frameGraph.AddPass<FakeWritterPass>("FakeWriter",
			[&](tiny::FrameGraph::Builder& b, FakeWritterPass& pass)
			{
				b.Write(finalTex);
			});


		return finalTex;
	}
};


int main()
{
	try
	{
		tiny::Initialize();
		std::cout << "Hello World!\n";

		DirectX::XMVECTOR cbuffer = { 1.0f, 0.4f, 0.0f, 1.0f };

		tiny::SerializedMaterialProperty prop;
		prop.propertyID = "color"_hs;
		prop.value = tiny::SerializedCBuffer{ std::vector<u8>(reinterpret_cast<u8*>(&cbuffer), reinterpret_cast<u8*>(&cbuffer) + sizeof(cbuffer)) };

		tiny::SerializedMaterial mat;
		mat.matID = std::hash<std::string>()("UnlitMaterial");
		mat.properties.push_back(prop);

		{
			std::ofstream oss("mat.json");
			cereal::JSONOutputArchive archive(oss);
			archive(mat);
		}

		// Deserialize
		tiny::SerializedMaterial deserializedMat;
		{
			std::ifstream ifs("mat.json");
			cereal::JSONInputArchive archive(ifs);
			archive(deserializedMat);
		}

		/*tiny::UnlitMaterialInstance unlit;

		Test2(&unlit);

		auto m = tiny::CreateMeshMaterialInstance2(deserializedMat);*/

		{
			{
				tiny::WindowDesc desc
				{
					.posX = 100,
					.posY = 100,
					.width = 1280,
					.height = 720,
					.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
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
				tiny::CreateRenderView(&w, new TinyEditorGraphBuilder());
				tiny::RunEngine();
			}
		}

		tiny::Shutdown();
	}
	catch (const tiny::DxException& e)
	{
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
	}
}