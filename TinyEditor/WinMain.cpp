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
		freopen("CONOUT$", "w", stdout);
		std::cout << "Console initialized" << std::endl;


		
		/*Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile("C:\\Users\\Alex\\Downloads\\glTF-Sample-Models-main\\2.0\\ABeautifulGame\\glTF\\ABeautifulGame.gltf", aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "Error loading model: " << importer.GetErrorString() << std::endl;
			return -1;
		}
		
		// Get num meshes
		if (scene->mNumMeshes == 0)
		{
			std::cout << "Error: No meshes found in the scene" << std::endl;
			return -1;
		}

		for (u32 i = 0; i < scene->mNumMeshes; ++i)
		{
			tiny::SerializedMesh serializedMesh;
			const aiMesh* mesh = scene->mMeshes[i];
			std::string meshName = mesh->mName.C_Str();
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

			std::string outputFileName = "Sponza\\" + meshName + ".asset";
			tiny::AssetSaveMesh(outputFileName.c_str(), serializedMesh);

			// Get diffuse texture
			if (mesh->mMaterialIndex >= 0)
			{
				const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
				aiString texturePath;
				if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
				{
					std::string textureFileName = texturePath.C_Str();
					std::string basePath = "C:\\Users\\Alex\\Downloads\\glTF-Sample-Models-main\\2.0\\ABeautifulGame\\glTF\\";
					std::string fullPath = basePath + textureFileName;
					
					tiny::SerializedMaterial unlitOpaqueStageMaterial;
					unlitOpaqueStageMaterial.matID = "UnlitMaterialInstance"_hs;
					unlitOpaqueStageMaterial.features = 0;
					tiny::SerializedTexture texture;
					i32 width, height, channels;
					u8* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);
					texture.width = width;
					texture.height = height;
					texture.depth = 1;
					texture.mipLevels = 1;
					texture.data.resize(width* height * 4);

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
					std::string techniqueFileName = "Sponza\\" + meshName + ".technique";
					tiny::AssetSaveTechnique(techniqueFileName, unlitOpaqueStageTechnique);
				}
			}
		}*/

		

		//tiny::SerializedMaterial unlitOpaqueStageMaterial;
		//unlitOpaqueStageMaterial.matID = "UnlitMaterialInstance"_hs;
		//unlitOpaqueStageMaterial.features = 0;

		//tiny::SerializedTexture texture;
		//i32 width, height, channels;
		//u8* data = stbi_load("C:\\Users\\Alex\\Downloads\\glTF-Sample-Models-main\\2.0\\DamagedHelmet\\glTF\\Default_albedo.png", &width, &height, &channels, 0);
		////u8* data = stbi_load("C:\\Users\\Alex\\Downloads\\qq.png", &width, &height, &channels, 0);
		//texture.width = width;
		//texture.height = height;
		//texture.depth = 1;
		//texture.mipLevels = 1;
		//texture.data.resize(width* height * 4);
		////memcpy(texture.data.data(), data, texture.data.size());

		/*if (channels == 3)
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
		tiny::AssetSaveTechnique("unlit.technique", unlitOpaqueStageTechnique);*/

		/*tiny::SerializedTechnique serializedTechnique = tiny::AssetLoadTechnique("unlit.technique");
		tiny::fx::MeshTechnique unlitTechnique = tiny::AssetCreateTechnique(serializedTechnique);

		tiny::SerializedMesh serializedMesh_ = tiny::AssetLoadMesh("mesh.asset");
		tiny::Mesh mesh_ = tiny::AssetCreateMesh(serializedMesh_);*/

		//tiny::Scene& scene = tiny::CreateScene();
		//auto object = scene.CreateObject("test object");

		//tiny::SerializedTechnique serializedTechnique = tiny::AssetLoadTechnique("unlit.technique");
		//tiny::fx::MeshTechnique unlitTechnique = tiny::AssetCreateTechnique(serializedTechnique);

		//tiny::SerializedMesh serializedMesh_ = tiny::AssetLoadMesh("mesh.asset");
		//tiny::Mesh mesh_ = tiny::AssetCreateMesh(serializedMesh_);

		//scene.Emplace<tiny::ComponentGeometry>(object, mesh_);
		//scene.Emplace<tiny::ComponentMaterial>(object, unlitTechnique);
		//scene.Emplace<tiny::ComponentTransform>(object);

		////scene.Destroy(object);

		//scene.Modify<tiny::ComponentTransform>(object, [&](tiny::ComponentTransform& transform)
		//{
		//		transform.position = { 0, 0, 0 };
		//		transform.rotation = { 0, 0, 0 };
		//		transform.scale = { 1, 1, 1 };
		//});


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
		w.Show();
		gRendererHost = std::make_unique<tiny::RendererHost>(w.hWnd);
		tiny::RenderView& view = tiny::CreateRenderView(&w, nullptr, gRendererHost.get());

        tiny::RunEngine();
		gRendererHost.reset();
    }
    tiny::Shutdown();
}