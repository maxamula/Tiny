#include "viewport.h"
#include "gfxglobal.h"

namespace tiny
{
	D3DViewport::D3DViewport(const D3DContext& context, HWND hWnd, u8 flags)
		: buffering(flags & D3DViewportFlags_TrippleBuffering ? 3 : 2), renderTargets({})
	{
		DXGI_SWAP_CHAIN_DESC1 scDesc
		{
			.Width = 0u,
			.Height = 0u,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.Stereo = false,
			.SampleDesc = { 1, 0 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = buffering,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
		};

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc
		{
			.RefreshRate = DXGI_RATIONAL{0},
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
			.Windowed = flags & D3DViewportFlags_Windowed ? TRUE : FALSE
		};

		CComPtr<IDXGISwapChain1> pSwap;
		THROW_IF_FAILED(gDxgiFactory->CreateSwapChainForHwnd(context.mainQueue.GetQueue(), hWnd, &scDesc, &fsDesc, NULL, &pSwap));
		THROW_IF_FAILED(pSwap.QueryInterface(&swap));


		for (u8 i = 0; i < buffering; ++i)
		{
			renderTargets[i].allocation = GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).AllocateDescriptor();
			THROW_IF_FAILED(swap->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&renderTargets[i].resource));
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				.Texture2D = { 0, 0 }
			};
			gDevice->CreateRenderTargetView(renderTargets[i].resource, &rtvDesc, renderTargets[i].allocation.GetCpuHandle());
		}
	}

	/*void D3DViewport::SetResolution(u16 width, u16 height)
	{
		if (!swap)
			THROW_ENGINE_EXCEPTION("D3DWindow - Swapchain is null");
		context->Flush();
		for (u8 i = 0; i < buffering; i++)
			renderTargets[i].resource.Release();
		u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		THROW_IF_FAILED(swap->ResizeBuffers(buffering, width, height, DXGI_FORMAT_UNKNOWN, flags));
		CreateRendertargetViews();
	}*/

	void D3DViewport::Present(u32 sync)
	{
		THROW_IF_FAILED(swap->Present(sync, sync == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0));
	}

	void D3DViewport::ReleaseRenderTargets()
	{
		for (u8 i = 0; i < buffering; ++i)
			renderTargets[i].resource.Release();
	}

	void D3DViewport::CreateRendertargetViews()
	{
		if (!swap)
			THROW_ENGINE_EXCEPTION("D3DWindow - Swapchain is null");

		for (u8 i = 0; i < buffering; ++i)
		{
			CComPtr<ID3D12Resource> pBackBuffer;
			THROW_IF_FAILED(swap->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&pBackBuffer));
			renderTargets[i].resource = pBackBuffer;
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				.Texture2D = { 0, 0 }
			};
			gDevice->CreateRenderTargetView(renderTargets[i].resource, &rtvDesc, renderTargets[i].allocation.GetCpuHandle());
		}
	}
}