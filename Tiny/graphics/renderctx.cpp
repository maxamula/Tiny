#include "renderctx.h"
#include "entt.hpp"
#include "content/content.h"
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
						cmd->SetGraphicsRoot32BitConstants(binding.rootParamIndex, size, data, 0);
					}
					else if (auto texture = value.try_cast<Texture>())
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
		auto it = fx->forward.pso.find(inputAttributes);
		if (it == fx->forward.pso.end())
			THROW_ENGINE_EXCEPTION("PSO not found for input attributes: {}", inputAttributes);

		context.Defer(it->second);
		context.Defer(fx->forward.rootSig);

		cmd->SetPipelineState(it->second);
		cmd->SetGraphicsRootSignature(fx->forward.rootSig);

		entt::meta_any meta = pMatInstance->Meta();
		entt::meta_type type = meta.type();

		for (auto binding : fx->forward.bindings)
		{
			entt::meta_data field = type.data(binding.fieldID);
			if (field)
			{
				entt::meta_any value = field.get(meta);
				if (value)
				{
					if (auto icbuffer = value.try_cast<ICBufferCPU>())
					{
						cmd->SetGraphicsRootConstantBufferView(binding.rootParamIndex, icbuffer->resource->GetGPUVirtualAddress());
						context.Defer(icbuffer->resource);
					}
					else if (auto texture = value.try_cast<Texture>())
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
	}
}