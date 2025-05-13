#include "materials.h"
#include "graphics/gfxcommon.h"
#include "fx/registry.h"
#include "meta.h"

using namespace entt;

TINY_REGISTER_MATERIAL(InitializeArt1Material, Art1MaterialInstance);
TINY_REGISTER_MATERIAL(InitializeArt2Material, Art2MaterialInstance);
TINY_REGISTER_MATERIAL(InitializeArt3Material, Art3MaterialInstance);
TINY_REGISTER_MATERIAL(InitializeArt4Material, Art4MaterialInstance);

META(Art1MaterialInstance)
{
	meta.base<tiny::fx::MaterialInstance<Art1MaterialInstance>>().ctor()
		.data<&Art1MaterialInstance::ilShaderParams>("ilShaderParams"_hs);
}

META(Art2MaterialInstance)
{
	meta.base<tiny::fx::MaterialInstance<Art2MaterialInstance>>().ctor()
		.data<&Art2MaterialInstance::ilShaderParams>("ilShaderParams"_hs);
}

META(Art3MaterialInstance)
{
	meta.base<tiny::fx::MaterialInstance<Art3MaterialInstance>>().ctor()
		.data<&Art3MaterialInstance::ilShaderParams>("ilShaderParams"_hs);
}

META(Art4MaterialInstance)
{
	meta.base<tiny::fx::MaterialInstance<Art4MaterialInstance>>().ctor()
		.data<&Art4MaterialInstance::ilShaderParams>("ilShaderParams"_hs);
}

void InitializeArt1Material(ID3D12Device* pDevice, tiny::fx::Material& material)
{
	tiny::fx::MaterialInitializationDesc desc
	{
		.fxDesc
		{
			.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art1.hlsl", "VSMain" },
			.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art1.hlsl", "PSMain" },
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

void InitializeArt2Material(ID3D12Device* pDevice, tiny::fx::Material& material)
{
	tiny::fx::MaterialInitializationDesc desc
	{
		.fxDesc
		{
			.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art2.hlsl", "VSMain" },
			.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art2.hlsl", "PSMain" },
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

void InitializeArt3Material(ID3D12Device* pDevice, tiny::fx::Material& material)
{
	tiny::fx::MaterialInitializationDesc desc
	{
		.fxDesc
		{
			.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art3.hlsl", "VSMain" },
			.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art3.hlsl", "PSMain" },
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

void InitializeArt4Material(ID3D12Device* pDevice, tiny::fx::Material& material)
{
	tiny::fx::MaterialInitializationDesc desc
	{
		.fxDesc
		{
			.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art4.hlsl", "VSMain" },
			.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\DemoShaderArtGallery\\shaders\\art4.hlsl", "PSMain" },
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