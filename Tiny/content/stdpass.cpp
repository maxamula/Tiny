//#include "stdpass.h"
//#include "content/content.h"
//#include "graphics/renderctx.h"
//#include "d3dx12.hpp"
//
//namespace tiny::fx
//{
//	CubeMapPassData CreateCubeMapPass(FrameGraph& frameGraph, FrameGraphResource cubeMap, FrameGraphResource depth)
//	{
//		return frameGraph.addCallbackPass<CubeMapPassData>("CubeMapPass",
//			[&](FrameGraph::Builder& builder, CubeMapPassData& data)
//			{
//				data.cubeMap = builder.read(cubeMap);
//				data.depth = builder.read(depth);
//				data.output = builder.create<RenderTexture>("CubeMapped", {});
//				data.output = builder.write(data.output);
//			},
//			[=](CubeMapPassData data, FrameGraph::PassResources& passRes, RenderContext* context)
//			{
//				RenderTexture output = passRes.get<RenderTexture>(data.output);
//				// barrier
//				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
//				context->cmd->ResourceBarrier(1, &barrier);
//
//				float color[4] = { 0.3f, 0.1f, 0.1f, 1.0f };
//				context->cmd->ClearRenderTargetView(output.rtv.cpu, color, 0, nullptr);
//			}
//		);
//	}
//
//	SubmitResultPassData CreateSubmitResultPass(FrameGraph& frameGraph, FrameGraphResource input)
//	{
//		return frameGraph.addCallbackPass<SubmitResultPassData>("SubmitResultPass",
//			[&](FrameGraph::Builder& builder, SubmitResultPassData& data)
//			{
//				data.finalTexture = builder.read(input);
//				data.material.SetFeatureFlags(0);
//			},
//			[=](SubmitResultPassData data, FrameGraph::PassResources& passRes, RenderContext* context)
//			{
//				RenderTexture finlaTexture = passRes.get<RenderTexture>(data.finalTexture);
//				data.material.diffuseTexture = finlaTexture;
//				float color[4] = { 0.8f, 0.8f, 0.7f, 1.0f };
//				context->cmd->ClearRenderTargetView(context->rtv, color, 0, nullptr);
//				context->BindMaterial(&data.material);
//				context->cmd->OMSetRenderTargets(1, &context->rtv, FALSE, nullptr);
//				D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 512.0f, 512.0f, 0.0f, 1.0f };
//				D3D12_RECT scissorRect = { 0, 0, 512, 512 };
//				context->cmd->RSSetViewports(1, &viewport);
//				context->cmd->RSSetScissorRects(1, &scissorRect);
//				context->cmd->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//				context->cmd->DrawInstanced(6, 1, 0, 0);	
//			}
//		);
//	}
//
//	TexProducerPassData CreateTexProducerPass(FrameGraph& frameGraph)
//	{
//		return frameGraph.addCallbackPass<TexProducerPassData>("TexProducerPass",
//			[&](FrameGraph::Builder& builder, TexProducerPassData& data)
//			{
//				RenderTexture::Desc desc
//				{
//					.width = 512,
//					.height = 512,
//					.format = DXGI_FORMAT_R8G8B8A8_UNORM,
//					.clearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.3f, 0.1f, 0.1f, 1.0f } }
//				};
//
//				data.output = builder.create<RenderTexture>("TexProducer", desc);
//				data.output = builder.write(data.output);
//			},
//			[=](TexProducerPassData data, FrameGraph::PassResources& passRes, RenderContext* context)
//			{
//				RenderTexture output = passRes.get<RenderTexture>(data.output);
//				// barrier
//				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
//				CD3DX12_RESOURCE_BARRIER::Transition(output.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
//				context->cmd->ResourceBarrier(1, &barrier);
//				float color[4] = { 0.3f, 0.1f, 0.1f, 1.0f };
//				context->cmd->ClearRenderTargetView(output.rtv.cpu, color, 0, nullptr);
//				// barrier
//				barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
//				context->cmd->ResourceBarrier(1, &barrier);
//			}
//		);
//	}
//}