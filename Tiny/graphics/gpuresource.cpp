#include "gpuresource.h"
#include "gfxglobal.h"
#include "d3dx12.hpp"



namespace tiny
{
	RenderTexture CreateRenderTexture(u32 width, u32 height, DXGI_FORMAT format)
	{
		RenderTexture texture;
		texture.rtv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).AllocateDescriptor();
		texture.srv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor();

		D3D12_RESOURCE_DESC desc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = width,
			.Height = height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = format,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};

		D3D12_CLEAR_VALUE clearValue
		{
			.Format = format,
			.Color = { 0.0f, 0.0f, 0.0f, 1.0f }
		};

		CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
		THROW_IF_FAILED(gDevice->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			initialState,
			&clearValue,
			IID_PPV_ARGS(&texture.resource)
		));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			}
		};

		gDevice->CreateShaderResourceView(texture.resource, &srvDesc, texture.srv.cpu);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
		{
			.Format = format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D
			{
				.MipSlice = 0,
				.PlaneSlice = 0
			}
		};
		gDevice->CreateRenderTargetView(texture.resource, &rtvDesc, texture.rtv.cpu);

		return texture;
	}
}