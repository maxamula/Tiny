#include "materials.h"
#include "fx/registry.h"
#include "meta.h"

namespace tiny
{

	TINY_REGISTER_MATERIAL(InitializeGridMaterial, GridMaterialInstance);

	META(GridMaterialInstance)
	{
		meta.base<fx::MaterialInstance<GridMaterialInstance>>().ctor();
	}

	void InitializeGridMaterial(ID3D12Device* pDevice, fx::Material& material)
	{
		const D3D12_DEPTH_STENCIL_DESC depthDesc
		{
			FALSE,								//BOOL DepthEnable;
			D3D12_DEPTH_WRITE_MASK_ZERO,		//D3D12_DEPTH_WRITE_MASK DepthWriteMask;
			D3D12_COMPARISON_FUNC_LESS_EQUAL,	//D3D12_COMPARISON_FUNC DepthFunc;
			FALSE,								//BOOL StencilEnable;
			0,									//UINT8 Stencil ReadMask;
			0,									//UINT8 StencilWriteMask;
			{},									//D3D12_DEPTH_STENCILOP_DESC FrontFace;
			{},									//D3D12_DEPTH_STENCILOP_DESC BackFace;
		};

		const D3D12_RASTERIZER_DESC NO_CULL
		{
			D3D12_FILL_MODE_SOLID,						// D3D12_FILL_MODE FillMode;
			D3D12_CULL_MODE_NONE,						// D3D12_CULL_MODE CullMode;
			0,											// BOOL FrontCounter Clockwise;
			0,											// INT DepthBias;
			0,											// FLOAT DepthBiasClamp;
			0,											// FLOAT Slope Scaled DepthBias;
			1,											// BOOL DepthClipEnable;
			1,											// BOOL MultisampleEnable;
			0,											// BOOL Antialiased LineEnable;
			0,											// UINT Forced SampleCount;
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	// D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
		};

		fx::MaterialInitializationDesc desc
		{
			.fxDesc
			{
				.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\TinyEditor\\shaders\\grid.hlsl", "VSMain" },
				.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\TinyEditor\\shaders\\grid.hlsl", "PSMain" },
				.defines = { },
				.numConditionalCompilations = 0u
			},
			.basePsoDesc
			{
				.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
				.SampleMask = UINT_MAX,
				.RasterizerState = NO_CULL,
				.DepthStencilState = depthDesc,
				.InputLayout = { nullptr, 0 },
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
				.SampleDesc = { 1, 0 }
			},
			.staticSamplers {}
		};

		MaterialDefaultInitialize(pDevice, desc, material);
	}
}