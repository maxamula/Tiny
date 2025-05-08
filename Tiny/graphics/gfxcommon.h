#pragma once
#include <d3d12.h>

namespace tiny
{
	const struct
	{
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

		const D3D12_RASTERIZER_DESC BACKFACE_CULL
		{
			D3D12_FILL_MODE_SOLID,						// D3D12_FILL_MODE FillMode;
			D3D12_CULL_MODE_BACK,						// D3D12_CULL_MODE CullMode;
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

		const D3D12_RASTERIZER_DESC FRONTFACE_CULL
		{
			D3D12_FILL_MODE_SOLID,						// D3D12_FILL_MODE FillMode;
			D3D12_CULL_MODE_FRONT,						// D3D12_CULL_MODE CullMode;
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

		const D3D12_RASTERIZER_DESC WIREFRAME
		{
			D3D12_FILL_MODE_WIREFRAME,						// D3D12_FILL_MODE FillMode;
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
	} RASTERIZER_STATE;

	const struct
	{
		const D3D12_DEPTH_STENCIL_DESC DISABLED
		{
			0,									//BOOL DepthEnable;
			D3D12_DEPTH_WRITE_MASK_ZERO,		//D3D12_DEPTH_WRITE_MASK DepthWriteMask;
			D3D12_COMPARISON_FUNC_LESS_EQUAL,	//D3D12_COMPARISON_FUNC DepthFunc;
			0,									//BOOL StencilEnable;
			0,									//UINT8 Stencil ReadMask;
			0,									//UINT8 StencilWriteMask;
			{},									//D3D12_DEPTH_STENCILOP_DESC FrontFace;
			{},									//D3D12_DEPTH_STENCILOP_DESC BackFace;
		};
	} DEPTH_STATE;

	const struct
	{
		const D3D12_HEAP_PROPERTIES DEFAULT
		{
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		};

		const D3D12_HEAP_PROPERTIES UPLOAD
		{
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		};
	} HEAP;
}