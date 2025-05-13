#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <chrono>
#include <functional>
#include <set>
#include <mutex>

#include "descriptors.h"
#include "gpuqueue.h"
#include "cbringbuffer.h"

namespace tiny
{
	struct D3DContext
	{
		D3DContext() = default;
		D3DContext(const D3DContext&) = delete;
		D3DContext& operator=(const D3DContext&) = delete;
		D3DContext(D3DContext&&) = delete;
		D3DContext& operator=(D3DContext&&) = delete;
		~D3DContext() = default;

		void Create(u8 framesInFlight);
		void Destroy();

		void BeginScene();
		void EndScene(std::function<void()> completionCallback);
		void Flush();

		void Defer(IUnknown* resource);
		void Defer(DescriptorHandle descriptor);
		void Defer(DescriptorRange range);

		D3DGpuQueue									mainQueue;
		FrameRingBuffer								frameRingBuffer;
		u8											framesInFlight;
		std::atomic<u8>								currentFrameIdx;
		std::vector<std::set<CComPtr<IUnknown>>>	deferredResources;
		std::mutex									deferredResourcesMutex;
		std::vector<std::set<DescriptorHandle>>		deferredDescriptors;
		std::mutex									deferredDescriptorsMutex;
		std::vector<std::set<DescriptorRange>>		deferredRanges;
		std::mutex									deferredRangesMutex;
	};
}