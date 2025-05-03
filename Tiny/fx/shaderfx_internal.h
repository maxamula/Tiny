#pragma once
#include <d3d12.h>

namespace tiny::fx
{
	void InitializeFX(ID3D12Device* pDevice);
	void ShutdonwnFX();
}