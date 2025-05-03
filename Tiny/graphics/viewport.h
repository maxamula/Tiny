#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atlbase.h>
#include <atomic>

#include "descriptors.h"
#include "d3dcontext.h"


namespace tiny
{
	enum D3DViewportFlags : u8
	{
		D3DViewportFlags_None = 0,
		D3DViewportFlags_TrippleBuffering = 1 << 0,
		D3DViewportFlags_Windowed = 1 << 1,
	};


	struct D3DViewport final
	{
		D3DViewport() = default;
		~D3DViewport() = default;
	_internal:
		struct RenderTarget
		{
			CComPtr<ID3D12Resource>		resource;
			DescriptorHandle			allocation;
		};

		D3DViewport(const D3DContext& context, HWND hWnd, u8 flags);

		void ReleaseRenderTargets();
		void CreateRendertargetViews();
		void Present(u32 sync);

		CComPtr<IDXGISwapChain4>	swap;
		u8							buffering;
		RenderTarget				renderTargets[TINY_MAX_SWAPCHAIN_BUFFERS];
	};
}