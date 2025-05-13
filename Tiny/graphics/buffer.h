#pragma once
#include <d3d12.h>
#include <atlbase.h>

namespace tiny
{
	class StructuredBuffer
	{
	public:

	private:
		CComPtr<ID3D12Resource> mBuffer;
		CComPtr<ID3D12Resource> mUploadBuffer;
	};
}