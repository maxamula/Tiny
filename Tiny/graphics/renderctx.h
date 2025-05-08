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
		void BindMaterial(fx::IMaterialInstance* pMatInstance);
		void BindMaterial(fx::IMeshMaterialInstance* pMatInstance, u8 inputAttributes);
		void BindMesh(const Mesh& mesh);
	_internal:
		entt::registry* renderItems;
		CComPtr<ID3D12GraphicsCommandList6> cmd;
		CBufferInline<WorldData> worldData;
	};
}