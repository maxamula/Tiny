#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <chrono>
#include <functional>

#include "descriptors.h"
#include "gpuqueue.h"

namespace tiny
{
	struct D3DContext
	{
		struct FrameTimingData
		{
			CComPtr<ID3D12QueryHeap>									timestampHeap;
			CComPtr<ID3D12Resource>										timestampResult;
			std::chrono::time_point<std::chrono::high_resolution_clock> cpuStart;
			std::chrono::time_point<std::chrono::high_resolution_clock> cpuEnd;
			f64															lastGpuTime;
			f64															lastCpuTime;
		};

		enum ContextState : u8
		{
			CONTEXT_STATE_IDLE,
			CONTEXT_STATE_DRAW,
			CONTEXT_STATE_SUBMITTED,
			CONTEXT_STATE_HALT
		};

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
		u8											framesInFlight;
		std::atomic<u8>								currentFrameIdx;
		std::vector<std::vector<CComPtr<IUnknown>>>	deferredResources;
		std::vector<std::vector<DescriptorHandle>>	deferredDescriptors;
		std::vector<std::vector<DescriptorRange>>	deferredRanges;
		std::vector<FrameTimingData>				frameTimingData;
	};
}