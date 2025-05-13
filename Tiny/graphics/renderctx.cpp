#include "renderctx.h"
#include "entt.hpp"
#include "fx/registry_internal.h"
#include "engine/application.h"

using namespace entt;

namespace tiny
{
	extern "C++" D3DContext gContext;

	void RenderContext::SetWorld(const DirectX::XMMATRIX& world)
	{
		this->world = world;
	}

	void RenderContext::BindMaterial(fx::IMaterialInstance* pMatInstance)
	{
		fx::ShaderFX* fx = pMatInstance->fx;
		cmd->SetPipelineState(fx->pso);
		cmd->SetGraphicsRootSignature(fx->rootSig);

		entt::meta_any meta = pMatInstance->Meta();
		entt::meta_type type = meta.type();

		for (fx::ShaderResourceBinding& binding : fx->bindings)
		{
			entt::meta_data field = type.data(binding.fieldID);
			if (field)
			{
				entt::meta_any value = field.get(meta);
				if (!value)
					THROW_ENGINE_EXCEPTION("Failed to get value for field {} in material type {}", binding.fieldID, pMatInstance->ID());
				switch (binding.type)
				{
				case D3D_SIT_CBUFFER:
				{
					entt::meta_any value = field.get(meta);
					void* data = value.data();
					u64 size = value.type().size_of();
					switch (binding.details.cbufferType)
					{
					case fx::ShaderCBufferType_Constants:
					{
						cmd->SetGraphicsRoot32BitConstants(binding.rootParamIndex, size / sizeof(u32), data, 0);
					}
					break;
					case fx::ShaderCBufferType_CPU:
					{
						FrameRingBuffer::Allocation alloc = gContext.frameRingBuffer.Allocate(size);
						std::memcpy(alloc.cpuAddress, data, size);
						cmd->SetGraphicsRootConstantBufferView(binding.rootParamIndex, alloc.gpuAddress);
					}
					break;
					case fx::ShaderCBufferType_GPU:
					{
						THROW_ENGINE_EXCEPTION("GPU cbuffer not supported yet");
					}
					}
				}
				break;
				case D3D_SIT_TEXTURE:
				{
					if (auto texture = value.try_cast<Texture2D>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, texture->srv.gpu);
						gContext.Defer(texture->resource);
						gContext.Defer(texture->srv);
					}
					else if (auto renderTexture = value.try_cast<RenderTexture>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, renderTexture->srv.gpu);
						gContext.Defer(renderTexture->resource);
						gContext.Defer(renderTexture->srv);
					}
				}
				break;
				}
			}
		}

		/*for (auto specialBinding : *pMatInstance->specialBindings)
		{
			if (specialBinding.special == TINY_SHADER_BIND_SPECIAL_WORLD_CBUFFER)
			{
				cmd->SetGraphicsRootConstantBufferView(specialBinding.rootParamIndex, gContext.worldCBuffer->resource->GetGPUVirtualAddress());
			}
			else if (specialBinding.special == TINY_SHADER_BIND_SPECIAL_LIGHT_CBUFFER)
			{
				cmd->SetGraphicsRootConstantBufferView(specialBinding.rootParamIndex, gContext.lightCBuffer->resource->GetGPUVirtualAddress());
			}
		}*/
	}

	void RenderContext::BindMaterial(fx::IMeshMaterialInstance* pMatInstance, u8 inputAttributes)
	{
		fx::MeshShaderFX* fx = pMatInstance->fx;
		auto it = fx->pso.find(inputAttributes);
		if (it == fx->pso.end())
			THROW_ENGINE_EXCEPTION("PSO not found for input attributes: {}", inputAttributes);

		gContext.Defer(it->second);
		gContext.Defer(fx->rootSig);

		cmd->SetPipelineState(it->second);
		cmd->SetGraphicsRootSignature(fx->rootSig);

		entt::meta_any meta = pMatInstance->Meta();
		entt::meta_type type = meta.type();

		for (auto binding : fx->bindings)
		{
			entt::meta_data field = type.data(binding.fieldID);
			if (field)
			{
				entt::meta_any value = field.get(meta);
				if (!value)
					THROW_ENGINE_EXCEPTION("Failed to get value for field {} in material type {}", binding.fieldID, pMatInstance->ID());
				switch (binding.type)
				{
				case D3D_SIT_CBUFFER:
				{
					entt::meta_any value = field.get(meta);
					void* data = value.data();
					u64 size = value.type().size_of();
					switch (binding.details.cbufferType)
					{
					case fx::ShaderCBufferType_Constants:
					{
						cmd->SetGraphicsRoot32BitConstants(binding.rootParamIndex, size / sizeof(u32), data, 0);
					}
					break;
					case fx::ShaderCBufferType_CPU:
					{
						FrameRingBuffer::Allocation alloc = gContext.frameRingBuffer.Allocate(size);
						std::memcpy(alloc.cpuAddress, data, size);
						cmd->SetGraphicsRootConstantBufferView(binding.rootParamIndex, alloc.gpuAddress);
					}
					break;
					case fx::ShaderCBufferType_GPU:
					{
						THROW_ENGINE_EXCEPTION("GPU cbuffer not supported yet");
					}
					}
				}
				break;
				case D3D_SIT_TEXTURE:
				{
					if (auto texture = value.try_cast<Texture2D>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, texture->srv.gpu);
						gContext.Defer(texture->resource);
						gContext.Defer(texture->srv);
					}
					else if (auto renderTexture = value.try_cast<RenderTexture>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, renderTexture->srv.gpu);
						gContext.Defer(renderTexture->resource);
						gContext.Defer(renderTexture->srv);
					}
				}
				break;
				}
			}
		}

		for (auto specialBinding : fx->specialBindings)
		{
			if (specialBinding.special == fx::ShaderSpecialBind_World)
			{
				struct alignas(16) WorldData
				{
					alignas(16) DirectX::XMMATRIX fullTransform;
				};
				WorldData worldData;
				
				if (renderView.scene->mCameras.size())
				{
					Scene::CameraItem camera = renderView.scene->mCameras[0];
					f32 fov = DirectX::XMConvertToRadians(camera.fov);
					f32 aspect = static_cast<f32>(renderView.window->width) / static_cast<f32>(renderView.window->height);
					DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, camera.nearZ, camera.farZ);
					worldData.fullTransform = DirectX::XMMatrixTranspose(world * camera.view * proj);						
				}
				else
				{
					auto view = DirectX::XMMatrixLookAtLH({ 5, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 });
					auto proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);
					worldData.fullTransform = DirectX::XMMatrixTranspose(world * view * proj);
				}
				FrameRingBuffer::Allocation alloc = gContext.frameRingBuffer.Allocate(sizeof(WorldData));
				std::memcpy(alloc.cpuAddress, &worldData, sizeof(WorldData));
				cmd->SetGraphicsRootConstantBufferView(specialBinding.rootParamIndex, alloc.gpuAddress);
			}
		}
	}

	void RenderContext::BindMesh(const Mesh& mesh)
	{
		cmd->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
		cmd->IASetIndexBuffer(&mesh.indexBufferView);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gContext.Defer(mesh.vertexBuffer);
		gContext.Defer(mesh.indexBuffer);
	}
}