#include "content.h"
#include "graphics/gfxglobal.h"
#include "d3dx12.hpp"

namespace tiny
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> ConstructInputLayout(u8 attributes)
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
		u32 offset = 0u;

		layout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		offset += 12;

		if (attributes & static_cast<u8>(InputAttribute_Normal))
		{
			layout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 12;
		}
		if (attributes & static_cast<u8>(InputAttribute_Tangent))
		{
			layout.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 12;
		}
		if (attributes & static_cast<u8>(InputAttribute_UV))
		{
			layout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 8;
		}
		if (attributes & static_cast<u8>(InputAttribute_Color))
		{
			layout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 16;
		}
		if (attributes & static_cast<u8>(InputAttribute_BoneWeights))
		{
			layout.push_back({ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 16;
		}
		if (attributes & static_cast<u8>(InputAttributes_BoneIndices))
		{
			layout.push_back({ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			offset += 16;
		}
		return layout;
	}

	u32 GetVertexStride(u8 attributes)
	{
		u32 stride = 12u; // Position always
		if (attributes & static_cast<u8>(InputAttribute_Normal))
			stride += 12;
		if (attributes & static_cast<u8>(InputAttribute_Tangent))
			stride += 12;
		if (attributes & static_cast<u8>(InputAttribute_UV))
			stride += 8;
		if (attributes & static_cast<u8>(InputAttribute_Color))
			stride += 16;
		if (attributes & static_cast<u8>(InputAttribute_BoneWeights))
			stride += 16;
		if (attributes & static_cast<u8>(InputAttributes_BoneIndices))
			stride += 16;
		return stride;
	}

	RenderTexture RenderTexture::Create(const Desc& desc)
	{
		RenderTexture tex;
		tex.rtv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).AllocateDescriptor();
		tex.srv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor();

		D3D12_RESOURCE_DESC resourceDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = desc.width,
			.Height = desc.height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = desc.format,
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
			.Format = desc.format,
			.Color = { 0.0f, 0.0f, 0.0f, 1.0f }
		};

		CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

		THROW_IF_FAILED(gDevice->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(&tex.resource)
		));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = desc.format,
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

		gDevice->CreateShaderResourceView(tex.resource, &srvDesc, tex.srv.cpu);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
		{
			.Format = desc.format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D
			{
				.MipSlice = 0,
				.PlaneSlice = 0
			}
		};
		gDevice->CreateRenderTargetView(tex.resource, &rtvDesc, tex.rtv.cpu);

		tex.width = desc.width;
		tex.height = desc.height;

		return tex;
	}

	DepthTexture DepthTexture::Create(const DepthTexture::Desc& desc)
	{
		DepthTexture tex;
		tex.dsv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).AllocateDescriptor();
		tex.srv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor();

		D3D12_RESOURCE_DESC resourceDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = desc.width,
			.Height = desc.height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		};

		D3D12_CLEAR_VALUE clearValue
		{
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			.DepthStencil
			{
				.Depth = 1.0f,
				.Stencil = 0
			}
		};

		CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
		THROW_IF_FAILED(gDevice->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			&clearValue,
			IID_PPV_ARGS(&tex.resource)
		));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
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

		gDevice->CreateShaderResourceView(tex.resource, &srvDesc, tex.srv.cpu);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
		{
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Texture2D
			{
				.MipSlice = 0
			}
		};
		gDevice->CreateDepthStencilView(tex.resource, &dsvDesc, tex.dsv.cpu);

		tex.width = desc.width;
		tex.height = desc.height;

		return tex;
	}
}