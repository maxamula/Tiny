#include "assets.h"
#include "fx/registry_internal.h"
#include "graphics/gfxglobal.h"
#include "graphics/copyqueue.h"
#include "entt.hpp"
#include "content.h"

namespace tiny
{
	TINYFX_API Mesh AssetCreateMesh(const SerializedMesh& mesh)
	{
		Mesh ret;
		u32 vertexBufferSize = mesh.vertices.size() * sizeof(f32);
		u32 indexBufferSize = mesh.indices.size() * sizeof(u32);
		ret.vertexBuffer = CreateGPUBuffer(vertexBufferSize, (void*)mesh.vertices.data(), D3D12_RESOURCE_STATE_GENERIC_READ);
		ret.indexBuffer = CreateGPUBuffer(indexBufferSize, (void*)mesh.indices.data(), D3D12_RESOURCE_STATE_GENERIC_READ);
		ret.vertexBufferView = D3D12_VERTEX_BUFFER_VIEW
		{
			.BufferLocation = ret.vertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = vertexBufferSize,
			.StrideInBytes = GetVertexStride(mesh.inputAttributes)
		};
		ret.indexBufferView = D3D12_INDEX_BUFFER_VIEW
		{
			.BufferLocation = ret.indexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = indexBufferSize,
			.Format = DXGI_FORMAT_R32_UINT
		};
		ret.inputAttributes = mesh.inputAttributes;
		ret.numIndices = mesh.indices.size();
		ret.numVertices = mesh.vertices.size() / GetVertexStride(mesh.inputAttributes) * sizeof(f32);
		return ret;
	}

	TINYFX_API Texture2D AssetCreateTexture(const SerializedTexture& texture)
	{
		Texture2D ret;
		D3D12_RESOURCE_DESC desc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = texture.width,
			.Height = texture.height,
			.DepthOrArraySize = 1,
			.MipLevels = texture.mipLevels,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		{
			CComPtr<ID3D12Resource> resource;
			CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
			D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;
			THROW_IF_FAILED(gDevice->CreateCommittedResource(
				&defaultHeap,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				initialState,
				nullptr,
				IID_PPV_ARGS(&resource)
			));

			CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
			CComPtr<ID3D12Resource> uploadResource;
			u32 rowPitch = desc.Width * 4;
			u32 slicePitch = rowPitch * desc.Height;
			u32 uploadBufferSize = slicePitch * desc.DepthOrArraySize;

			auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			THROW_IF_FAILED(gDevice->CreateCommittedResource(
				&uploadHeap,
				D3D12_HEAP_FLAG_NONE,
				&uploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadResource)
			));

			D3D12_SUBRESOURCE_DATA subresourceData
			{
				.pData = texture.data.data(),
				.RowPitch = static_cast<LONG_PTR>(rowPitch),
				.SlicePitch = static_cast<LONG_PTR>(slicePitch)
			};

			GetEngineCopyQueue().UpdateSubresource(resource, uploadResource.p, 0, 0, 1, &subresourceData, D3D12_RESOURCE_STATE_COMMON).wait();
			ret.resource = resource;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MostDetailedMip = 0,
				.MipLevels = texture.mipLevels,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			}
		};

		ret.srv = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).AllocateDescriptor();
		gDevice->CreateShaderResourceView(ret.resource, &srvDesc, ret.srv.cpu);
		return ret;
	}
}