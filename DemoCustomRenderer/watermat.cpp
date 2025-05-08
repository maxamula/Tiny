#include "watermat.h"
#include "graphics/gfxcommon.h"
#include "fx/registry.h"
#include "meta.h"

using namespace entt;

TINY_REGISTER_MATERIAL(InitializeWaterMaterial, WaterMaterialInstance);

META(WaterMaterialInstance)
{
	meta.base<tiny::fx::MaterialInstance<WaterMaterialInstance>>().ctor()
		.data<&WaterMaterialInstance::ilShaderParams>("ilShaderParams"_hs);
}

void InitializeWaterMaterial(ID3D12Device* pDevice, tiny::fx::Material& material)
{
	tiny::fx::MaterialInitializationDesc desc
	{
		.fxDesc
		{
			.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoCustomRenderer\\shaders\\fullscreen_water.hlsl", "VSMain" },
			.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoCustomRenderer\\shaders\\fullscreen_water.hlsl", "PSMain" },
			.defines = { },
			.numConditionalCompilations = 0u
		},
		.basePsoDesc
		{
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = tiny::RASTERIZER_STATE.NO_CULL,
			.DepthStencilState = tiny::DEPTH_STATE.DISABLED,
			.InputLayout = { nullptr, 0 },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
			.SampleDesc = { 1, 0 }
		}
	};

	MaterialDefaultInitialize(pDevice, desc, material);
}