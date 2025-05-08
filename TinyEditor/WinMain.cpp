#include <Windows.h>
#include <engine/application.h>
#include "rendering/rendererhost.h"
#include <content/assets.h>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<tiny::RendererHost> gRendererHost;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    tiny::Initialize();
    {
		AllocConsole();
		FILE* pFile;
		freopen_s(&pFile, "CONOUT$", "w", stdout);


		/*tiny::SerializedMesh serializedMesh;
		//// use assimp to load the mesh
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile("C:\\Users\\Alex\\Downloads\\glTF-Sample-Models-main\\2.0\\DamagedHelmet\\glTF\\DamagedHelmet.gltf", aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "Error loading model: " << importer.GetErrorString() << std::endl;
			return -1;
		}
		
		const aiMesh* mesh = scene->mMeshes[0];
		u8 inputAttributes = 0;

		if (!mesh->HasPositions())
		{
			std::cout << "Error: Mesh has no positions" << std::endl;
			return -1;
		}

		u32 stride = 3;			
		if (mesh->HasTextureCoords(0))
		{
			inputAttributes |= tiny::InputAttribute_UV;
			stride += 2;
		}


		serializedMesh.vertices.resize(mesh->mNumVertices * stride);
		serializedMesh.inputAttributes = inputAttributes;

		for (u32 i = 0; i < mesh->mNumVertices; ++i) {
			u32 offset = i * stride;

			if (mesh->HasPositions()) {
				serializedMesh.vertices[offset++] = mesh->mVertices[i].x;
				serializedMesh.vertices[offset++] = mesh->mVertices[i].y;
				serializedMesh.vertices[offset++] = mesh->mVertices[i].z;
			}

			if (mesh->HasTextureCoords(0)) {
				serializedMesh.vertices[offset++] = mesh->mTextureCoords[0][i].x;
				serializedMesh.vertices[offset++] = mesh->mTextureCoords[0][i].y;
			}
		}

		serializedMesh.indices.resize(mesh->mNumFaces * 3);
		for (u32 i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			if (face.mNumIndices != 3) {
				std::cout << "Error: Only triangles are supported" << std::endl;
				return -1;
			}
			serializedMesh.indices[i * 3] = face.mIndices[0];
			serializedMesh.indices[i * 3 + 1] = face.mIndices[1];
			serializedMesh.indices[i * 3 + 2] = face.mIndices[2];
		}


		tiny::AssetSaveMesh("mesh.asset", serializedMesh);

		tiny::SerializedMaterial unlitOpaqueStageMaterial;
		unlitOpaqueStageMaterial.matID = "UnlitMaterialInstance"_hs;
		unlitOpaqueStageMaterial.features = 0;

		tiny::SerializedTexture texture;
		i32 width, height, channels;
		u8* data = stbi_load("C:\\Users\\Alex\\Downloads\\glTF-Sample-Models-main\\2.0\\DamagedHelmet\\glTF\\Default_albedo.png", &width, &height, &channels, 0);
		//u8* data = stbi_load("C:\\Users\\Alex\\Downloads\\qq.png", &width, &height, &channels, 0);
		texture.width = width;
		texture.height = height;
		texture.depth = 1;
		texture.mipLevels = 1;
		texture.data.resize(width* height * 4);
		////memcpy(texture.data.data(), data, texture.data.size());

		if (channels == 3)
		{
			for (int i = 0; i < width * height; ++i)
			{
				texture.data[i * 4] = data[i * 3];
				texture.data[i * 4 + 1] = data[i * 3 + 1];
				texture.data[i * 4 + 2] = data[i * 3 + 2];
				texture.data[i * 4 + 3] = 255;
			}
		}
		else if (channels == 4)
		{
			memcpy(texture.data.data(), data, width * height * 4);
		}

		stbi_image_free(data);
		unlitOpaqueStageMaterial.properties.push_back({ "diffuseTexture"_hs, texture });

		tiny::SerializedTechnique unlitOpaqueStageTechnique;
		unlitOpaqueStageTechnique.materials[tiny::fx::MeshTechnique::RenderPassId_Opaque] = unlitOpaqueStageMaterial;
		tiny::AssetSaveTechnique("unlit.technique", unlitOpaqueStageTechnique);

		tiny::SerializedTechnique serializedTechnique = tiny::AssetLoadTechnique("unlit.technique");
		tiny::fx::MeshTechnique unlitTechnique = tiny::AssetCreateTechnique(serializedTechnique);

		tiny::SerializedMesh serializedMesh_ = tiny::AssetLoadMesh("mesh.asset");
		tiny::Mesh mesh_ = tiny::AssetCreateMesh(serializedMesh_);*/

		tiny::Scene& scene = tiny::GetScene();
		auto object = scene.CreateObject("test object");

		tiny::SerializedTechnique serializedTechnique = tiny::AssetLoadTechnique("unlit.technique");
		tiny::fx::MeshTechnique unlitTechnique = tiny::AssetCreateTechnique(serializedTechnique);

		tiny::SerializedMesh serializedMesh_ = tiny::AssetLoadMesh("mesh.asset");
		tiny::Mesh mesh_ = tiny::AssetCreateMesh(serializedMesh_);

		scene.GetRegistry().emplace<tiny::ComponentGeometry>(object, mesh_);
		scene.GetRegistry().emplace<tiny::ComponentMaterial>(object, unlitTechnique);

		tiny::WindowDesc desc
		{
			.posX = 100,
			.posY = 100,
			.width = 1280,
			.height = 720,
			.proc = [](tiny::Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
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
		tiny::RenderView* view = tiny::CreateRenderView(&w, gRendererHost.get());
		w.Resized.connect([&](u16 width, u16 height)
		{
			view->bMarkedForRebuild = true;
		});
        tiny::RunEngine();
		gRendererHost.reset();
    }
    tiny::Shutdown();
}