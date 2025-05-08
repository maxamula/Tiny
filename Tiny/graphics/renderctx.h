#pragma once
#include "fx/shaderfx.h"
#include "d3dcontext.h"
#include "content/content.h"

namespace tiny
{
	struct Mesh;

	struct TINYFX_API RenderContext
	{
		struct alignas(16) WorldData
		{
			alignas(16) DirectX::XMMATRIX fullTransform;
		};
		//void SetRenderTarget(RenderTexture& rt);
		//void SetScissorAndViewport(const D3D12_VIEWPORT& vp, const D3D12_RECT& scissor);
		void BindMaterial(fx::IMaterialInstance* pMatInstance);
		void BindMaterial(fx::IMeshMaterialInstance* pMatInstance, u8 inputAttributes);
		void BindMesh(const Mesh& mesh);
	_internal:
		D3DContext& context;
		entt::registry* renderItems;
		CComPtr<ID3D12GraphicsCommandList6> cmd;
		CBufferCPU<WorldData> worldData;
	};
}