#include "gfxglobal.h"
#include <atlbase.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")


namespace tiny
{
	ID3D12Device8* gDevice			{ nullptr };
	IDXGIFactory7* gDxgiFactory		{ nullptr };
	IDXGIAdapter4* gDxgiAdapter		{ nullptr };

	void InitializeGFX()
	{
		THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&gDxgiFactory)));
		CComPtr<IDXGIAdapter1> adapter = nullptr;
		for (UINT i = 0; gDxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			THROW_IF_FAILED(adapter->GetDesc1(&adapterDesc));
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			if (SUCCEEDED(D3D12CreateDevice(adapter.p, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gDevice))))
			{
				THROW_IF_FAILED(adapter->QueryInterface(IID_PPV_ARGS(&gDxgiAdapter)));
				return;
			}
		}
		THROW_ENGINE_EXCEPTION("Suita bleadapter not found");
	}

	void ShutdownGFX()
	{
		SAFE_RELEASE(gDxgiAdapter);
		SAFE_RELEASE(gDxgiFactory);
		SAFE_RELEASE(gDevice);
	}
}