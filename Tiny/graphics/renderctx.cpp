#include "renderctx.h"
#include "entt.hpp"
#include "fx/registry_internal.h"
using namespace entt;

namespace tiny
{
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
				if (value)
				{
					if (auto icbuffer = value.try_cast<ICBuffer>())
					{
						//cmd->SetGraphicsRootConstantBufferView(binding.rootParamIndex, icbuffer->resource->GetGPUVirtualAddress());
						auto [data, size] = icbuffer->GetData();
						cmd->SetGraphicsRoot32BitConstants(binding.rootParamIndex, size / sizeof(u32), data, 0);
					}
					else if (auto texture = value.try_cast<Texture2D>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, texture->srv.gpu);
					}
					else if (auto renderTexture = value.try_cast<RenderTexture>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, renderTexture->srv.gpu);

						auto gpu = renderTexture->srv.gpu;
						auto cpu = renderTexture->srv.cpu;

						context.Defer(renderTexture->rtv);
						context.Defer(renderTexture->srv);
						context.Defer(renderTexture->resource);
					}
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

		context.Defer(it->second);
		context.Defer(fx->rootSig);

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
				if (value)
				{
					if (auto icbuffer = value.try_cast<CBufferCPUBase>())
					{
						cmd->SetGraphicsRootConstantBufferView(binding.rootParamIndex, icbuffer->resource->GetGPUVirtualAddress());
						context.Defer(icbuffer->resource);
					}
					else if (auto texture = value.try_cast<Texture2D>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, texture->srv.gpu);
						context.Defer(texture->resource);
						context.Defer(texture->srv);
					}
					else if (auto renderTexture = value.try_cast<RenderTexture>())
					{
						cmd->SetGraphicsRootDescriptorTable(binding.rootParamIndex, renderTexture->srv.gpu);
						context.Defer(renderTexture->resource);
						context.Defer(renderTexture->srv);
					}
				}
			}
		}

		for (auto specialBinding : fx->specialBindings)
		{
			if (specialBinding.special == fx::ShaderSpecialBind_World)
			{
				// TODO hardcoded
				/*worldData.data.world = DirectX::XMMatrixIdentity();
				worldData.data.view = DirectX::XMMatrixLookAtRH({ 0, 0, -5 }, { 0, 0, 0 }, { 0, 1, 0 });
				worldData.data.proj = DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);
				worldData.data.fullTransform = DirectX::XMMatrixTranspose(worldData.data.world * worldData.data.view * worldData.data.proj);
				worldData.Update();*/
				//worldData.data.fullTransform = view * projection;

				auto world = DirectX::XMMatrixIdentity();
				static float angle = 0.0f;
				angle += 0.01f;
				world = DirectX::XMMatrixRotationY(angle);

				auto view = DirectX::XMMatrixLookAtLH({ 5, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 });
				auto proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);
				auto fullTransform = DirectX::XMMatrixTranspose(world * view * proj);
				//cmd->SetGraphicsRoot32BitConstants(specialBinding.rootParamIndex, sizeof(fullTransform) / sizeof(u32), &fullTransform, 0);

				worldData.data.fullTransform = fullTransform;
				worldData.Update();
				cmd->SetGraphicsRootConstantBufferView(specialBinding.rootParamIndex, worldData.resource->GetGPUVirtualAddress());
				context.Defer(worldData.resource);

				/*auto [data, size] = worldData.GetData();
				cmd->SetGraphicsRootConstantBufferView(specialBinding.rootParamIndex, worldData.resource->GetGPUVirtualAddress());
				context.Defer(worldData.resource);*/
			}
		}
	}

	void RenderContext::BindMesh(const Mesh& mesh)
	{
		cmd->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
		cmd->IASetIndexBuffer(&mesh.indexBufferView);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.Defer(mesh.vertexBuffer);
		context.Defer(mesh.indexBuffer);
	}
}