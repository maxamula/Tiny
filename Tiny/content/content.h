#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <tuple>
#include "graphics/descriptors.h"
#include "graphics/copyqueue.h"
#include "entt.hpp"
//#include "graphics/renderctx.h"

using namespace entt;

namespace tiny
{
	enum InputAttributes
	{
		InputAttribute_Position = 0,
		InputAttribute_Normal = 1 << 0,
		InputAttribute_Tangent = 1 << 1,
		InputAttribute_UV = 1 << 2,
		InputAttribute_Color = 1 << 3,
		InputAttribute_BoneWeights = 1 << 4,
		InputAttributes_BoneIndices = 1 << 5,
		NumInputAttributes = 7
	};

	struct TINYFX_API Texture2D
	{
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle srv;
		u32 width, height;
	};

	struct TINYFX_API RenderTexture
	{
		struct Desc
		{
			u32 width;
			u32 height;
			DXGI_FORMAT format;
			D3D12_CLEAR_VALUE clearValue;
		};
		static RenderTexture Create(const Desc& desc);
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle rtv;
		DescriptorHandle srv;
		u32 width, height;
	};

	struct TINYFX_API DepthTexture
	{
		struct Desc
		{
			u32 width;
			u32 height;
		};
		static DepthTexture Create(const Desc& desc);
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle dsv;
		DescriptorHandle srv;
		u32 width, height;
	};

	struct TINYFX_API Mesh
	{
	_internal:
		CComPtr<ID3D12Resource> vertexBuffer;
		CComPtr<ID3D12Resource> indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		u32 numIndices;
		u32 numVertices;
		u8 inputAttributes;
	};

	std::vector<D3D12_INPUT_ELEMENT_DESC> ConstructInputLayout(u8 attributes);
	TINYFX_API u32 GetVertexStride(u8 attributes);
}