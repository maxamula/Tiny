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
		this->deferredResources.resize(framesInFlight);
		this->deferredDescriptors.resize(framesInFlight);
		this->deferredRanges.resize(framesInFlight);
		this->framesInFlight = framesInFlight;
		this->currentFrameIdx = 0;

		this->frameTimingData.resize(framesInFlight);
		for (auto& frame : frameTimingData)
		{
			D3D12_QUERY_HEAP_DESC queryHeapDesc
			{
				.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
				.Count = 2,
				.NodeMask = 0
			};

			THROW_IF_FAILED(gDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&frame.timestampHeap)));

			// Create a readback buffer for the query data
			auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(u64));
			auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
			THROW_IF_FAILED(gDevice->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&frame.timestampResult)
			));
		}
	}

	void D3DContext::Destroy()
	{
		Flush();
		mainQueue.Destroy();
		for (auto& frame : frameTimingData)
		{
			frame.timestampHeap.Release();
			frame.timestampResult.Release();
		}
		frameTimingData.clear();
		deferredResources.clear();
		deferredDescriptors.clear();
		deferredRanges.clear();
	}

	void D3DContext::BeginScene()
	{
		mainQueue.BeginFrame();

		std::cout << "Going to clear deferred resources: " << deferredResources[currentFrameIdx].size() << " at index: " << (int)currentFrameIdx.load() << "\n";
		deferredResources[currentFrameIdx].clear();
		deferredDescriptors[currentFrameIdx].clear();
		deferredRanges[currentFrameIdx].clear();



		// Begin the frame
		

		// Record profiling data
		/*auto& frameData = frameTimingData[currentFrameIdx];
		frameData.lastCpuTime = std::chrono::duration<double, std::milli>(frameData.cpuEnd - frameData.cpuStart).count();
		u64 queueFreq = 0;
		ThrowIfFailed(mainQueue->GetTimestampFrequency(&queueFreq));
		if (frameData.timestampHeap)
		{
			u64* timestampData;
			D3D12_RANGE readRange = { 0, 2 * sizeof(u64) };
			ThrowIfFailed(frameData.timestampResult->Map(0, &readRange, (void**)&timestampData));
			frameData.lastGpuTime = (timestampData[1] - timestampData[0]) * 1000.0 / queueFreq;
			frameData.timestampResult->Unmap(0, nullptr);
		}*/

		//D3DGpuQueue::CommandFrame& cmdFrame = mainQueue.GetCurrentCommandFrame();
		//frameData.cpuStart = std::chrono::high_resolution_clock::now();
		//cmdFrame.cmdContexts[0].commandList->EndQuery(frameData.timestampHeap.p, D3D12_QUERY_TYPE_TIMESTAMP, 0); // Set timestamp for the first command list, as the first command list is the one that will be submitted first
	}

	void D3DContext::EndScene(std::function<void()> completionCallback)
	{

		//auto& frameData = frameTimingData[currentFrameIdx];
		/*D3DGpuQueue::CommandFrame& cmdFrame = mainQueue.GetCurrentCommandFrame();
		cmdFrame.cmdContexts[mainQueue.GetCommandsPerFrame() - 1].commandList->EndQuery(frameData.timestampHeap.p, D3D12_QUERY_TYPE_TIMESTAMP, 1);
		cmdFrame.cmdContexts[mainQueue.GetCommandsPerFrame() - 1].commandList->ResolveQueryData(frameData.timestampHeap.p, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, frameData.timestampResult.p, 0);
		frameData.cpuEnd = std::chrono::high_resolution_clock::now();*/

		currentFrameIdx = (currentFrameIdx + 1) % framesInFlight;

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
