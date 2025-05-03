#pragma once
#include "content/content.h"
#include "fx/shaderfx.h"
#include "d3dcontext.h"

namespace tiny
{
	struct WorldBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		DirectX::XMMATRIX fullTransform;
	};

	struct LightBuffer
	{

	};

	struct RenderContext
	{
		void BindMaterial(fx::IMaterialInstance* pMatInstance);
		void BindMaterial(fx::IMeshMaterialInstance* pMatInstance, u8 inputAttributes);
		//void SetGeometry(const Mesh& mesh);
		//entt::registry& GetRenderItems() { return gScene.registry; }

	_internal:
		D3DContext& context;
		entt::registry* renderItems;
		CComPtr<ID3D12GraphicsCommandList6> cmd;
	};
}