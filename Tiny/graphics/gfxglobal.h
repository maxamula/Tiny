#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

namespace tiny
{
	extern "C++" TINYFX_API ID3D12Device8* gDevice;
	extern "C++" TINYFX_API IDXGIFactory7* gDxgiFactory;
	extern "C++" TINYFX_API IDXGIAdapter4* gDxgiAdapter;

	void InitializeGFX();
	void ShutdownGFX();
}