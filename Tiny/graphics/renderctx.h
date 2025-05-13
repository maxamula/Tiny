#pragma once
#include "fx/shaderfx.h"
#include "d3dcontext.h"
#include "content/content.h"

namespace tiny
{
	struct Mesh;
	struct RenderView;

	struct TINYFX_API RenderContext
	{
		void SetWorld(const DirectX::XMMATRIX& world);

		void BindMaterial(fx::IMaterialInstance* pMatInstance);
		void BindMaterial(fx::IMeshMaterialInstance* pMatInstance, u8 inputAttributes);
		void BindMesh(const Mesh& mesh);
	_internal:
		CComPtr<ID3D12GraphicsCommandList6> cmd;
		RenderView&							renderView;
		DirectX::XMMATRIX					world;
	};
}