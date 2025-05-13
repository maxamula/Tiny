#include "d3dcontext.h"
#include "gfxglobal.h"
#include "d3dx12.hpp"

namespace tiny
{
	void D3DContext::Create(u8 framesInFlight)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc
		{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};
		this->mainQueue.Create(queueDesc, framesInFlight);
		this->frameRingBuffer.Initialize(gDevice, mainQueue.GetQueue(), 256 * 1024 * 1024); // 256 MB ring buffer
		this->deferredResources.resize(framesInFlight);
		this->deferredDescriptors.resize(framesInFlight);
		this->deferredRanges.resize(framesInFlight);
		this->framesInFlight = framesInFlight;
		this->currentFrameIdx = 0;
	}

	void D3DContext::Destroy()
	{
		Flush();
		frameRingBuffer.Destroy();
		mainQueue.Destroy();
		deferredResources.clear();
		deferredDescriptors.clear();
		deferredRanges.clear();
	}

	void D3DContext::BeginScene()
	{
		currentFrameIdx = mainQueue.BeginFrame();
		deferredResources[currentFrameIdx].clear();
		deferredDescriptors[currentFrameIdx].clear();
		deferredRanges[currentFrameIdx].clear();
	}

	void D3DContext::EndScene(std::function<void()> completionCallback)
	{
		mainQueue.EndFrame(completionCallback);
	}

	void D3DContext::Flush()
	{
		if (framesInFlight)
		{
			mainQueue.Flush();

			for (u8 i = 0; i < framesInFlight; ++i)
			{
				deferredResources[i].clear();
				deferredDescriptors[i].clear();
				deferredRanges[i].clear();
			}
		}
	}

	void D3DContext::Defer(IUnknown* resource)
	{
		std::lock_guard lock(deferredResourcesMutex);
		deferredResources[currentFrameIdx].insert(resource);
	}

	void D3DContext::Defer(DescriptorHandle descriptor)
	{
		std::lock_guard lock(deferredDescriptorsMutex);
		deferredDescriptors[currentFrameIdx].insert(descriptor);
	}

	void D3DContext::Defer(DescriptorRange range)
	{
		std::lock_guard lock(deferredRangesMutex);
		deferredRanges[currentFrameIdx].insert(range);
	}
}
