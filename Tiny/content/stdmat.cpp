#include "stdmat.h"
#include "fx/shaderfx.h"
#include "fx/registry.h"
#include "graphics/gfxcommon.h"
#include "meta.h"

using namespace tiny::fx;
using namespace entt;

namespace tiny
{
	TINY_REGISTER_MATERIAL(InitializeBeamMaterial, BeamMaterialInstance);
	TINY_REGISTER_MATERIAL(InitializeTextureStretcherMaterial, TextureStretcherMaterialInstance);
	TINY_REGISTER_MESH_MATERIAL(InitializeUnlitMaterial, UnlitMaterialInstance);

	META(BeamMaterialInstance)
	{
		meta.base<MaterialInstance<BeamMaterialInstance>>().ctor()
			.data<&BeamMaterialInstance::ilShaderParams>("ilShaderParams"_hs);
	}

	META(TextureStretcherMaterialInstance)
	{
		meta.base<MaterialInstance<TextureStretcherMaterialInstance>>().ctor()
			.data<&TextureStretcherMaterialInstance::diffuseTexture>("diffuseTexture"_hs);
	}

	META(UnlitMaterialInstance)
	{
		meta.base<fx::MeshMaterialInstance<UnlitMaterialInstance>>().ctor()
			.data<&UnlitMaterialInstance::diffuseTexture>("diffuseTexture"_hs);
	}



	void InitializeBeamMaterial(ID3D12Device* pDevice, Material& material)
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
				.vs = { "C:\\Users\\Alex\\source\\repos\\Tiny\\Tiny\\shaders\\beam.hlsl", "VSMain" },
				.ps = { "C:\\Users\\Alex\\source\\repos\\Tiny\\Tiny\\shaders\\beam.hlsl", "PSMain" },
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

	void InitializeTextureStretcherMaterial(ID3D12Device* pDevice, Material& material)
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

		D3D12_STATIC_SAMPLER_DESC FullScreenSampler = {};
		FullScreenSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;            // линейное смешение по всем трём уровням
		FullScreenSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;          // не выходить за границы [0,1]
		FullScreenSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		FullScreenSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		FullScreenSampler.MipLODBias = 0;                                         // без смещения LOD
		FullScreenSampler.MaxAnisotropy = 0;                                         // анизотропия не нужна
		FullScreenSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;              // игнорируется для обычных фильтров
		FullScreenSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;    // не используется при clamp
		FullScreenSampler.MinLOD = 0.0f;
		FullScreenSampler.MaxLOD = D3D12_FLOAT32_MAX;
		FullScreenSampler.ShaderRegister = 0;                                         // слот s0
		FullScreenSampler.RegisterSpace = 0;
		FullScreenSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


		CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		fx::MaterialInitializationDesc desc
		{
			.fxDesc
			{
				.vs = { "res:Tiny::fssampler", "VSMain" },
				.ps = { "res:Tiny::fssampler", "PSMain" },
				.defines = { },
				.numConditionalCompilations = 0u
			},
			.basePsoDesc
			{
				.BlendState = blendDesc,
				.SampleMask = UINT_MAX,
				.RasterizerState = NO_CULL,
				.DepthStencilState = depthDesc,
				.InputLayout = { nullptr, 0 },
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
				.SampleDesc = { 1, 0 }
			},
			.staticSamplers
			{
				StaticSamplerDesc
				{
					.id = "linearSampler"_hs,
					.desc = FullScreenSampler
				}
			}
		};

		MaterialDefaultInitialize(pDevice, desc, material);
	}

	void InitializeUnlitMaterial(ID3D12Device* pDevice, MeshMaterial& material)
	{
		D3D12_STATIC_SAMPLER_DESC linearClampSampler = {};
		linearClampSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		linearClampSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		linearClampSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		linearClampSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		linearClampSampler.MipLODBias = 0;
		linearClampSampler.MaxAnisotropy = 1;
		linearClampSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		linearClampSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		linearClampSampler.MinLOD = 0.0f;
		linearClampSampler.MaxLOD = D3D12_FLOAT32_MAX;
		linearClampSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		const D3D12_DEPTH_STENCIL_DESC depthDesc
		{
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
			.StencilEnable = FALSE,
		};

		fx::MaterialInitializationDesc desc
		{
			.fxDesc
			{
				.vs = { "res:Tiny::mesh_unlit", "VSMain" },
				.ps = { "res:Tiny::mesh_unlit", "PSMain" },
				.defines = { },
				.numConditionalCompilations = 0u
			},
			.basePsoDesc
			{
				.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
				.SampleMask = UINT_MAX,
				.RasterizerState = RASTERIZER_STATE.NO_CULL,
				.DepthStencilState = depthDesc,
				.InputLayout = { nullptr, 0 },
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
				.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
				.SampleDesc = { 1, 0 }
			},
			.staticSamplers
			{
				StaticSamplerDesc
				{
					.id = "linearSampler"_hs,
					.desc = linearClampSampler
				}
			}
		};

		MeshMaterialDefaultInitialize(pDevice, desc, material);
	}
}