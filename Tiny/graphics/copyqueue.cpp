#include "copyqueue.h"
#include "gfxglobal.h"
#include "d3dx12.hpp"


namespace tiny
{
	CopyQueue::CopyQueue(u8 bufferCount)
		: mCopyFrames(bufferCount), mFenceEvent(CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS)),
		mFrameIndex(0), mNextFenceValue(1), mFramesInFlight(bufferCount)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc
		{
			.Type = D3D12_COMMAND_LIST_TYPE_COPY,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};

		THROW_IF_FAILED(gDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

		for (u8 i = 0; i < bufferCount; ++i)
		{
			THROW_IF_FAILED(gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyFrames[i].commandAllocator)));
			THROW_IF_FAILED(gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyFrames[i].commandAllocator.p, nullptr, IID_PPV_ARGS(&mCopyFrames[i].commandList)));
			THROW_IF_FAILED(mCopyFrames[i].commandList->Close());
			mCopyFrames[i].fenceValue = 0;
		}

		THROW_IF_FAILED(gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	}

	std::future<void> CopyQueue::CopyResource(ID3D12Resource* pDstResource, ID3D12Resource* pSrcResource)
	{
		u8 currentFrameIndex = mFrameIndex;

		_WaitForFrame(currentFrameIndex);

		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandAllocator->Reset());
		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandList->Reset(mCopyFrames[currentFrameIndex].commandAllocator.p, nullptr));

		mCopyFrames[currentFrameIndex].commandList->CopyResource(pDstResource, pSrcResource);

		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { mCopyFrames[currentFrameIndex].commandList.p };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		u64 fenceValue = mNextFenceValue;
		THROW_IF_FAILED(mCommandQueue->Signal(mFence.p, fenceValue));
		mCopyFrames[currentFrameIndex].fenceValue = fenceValue;
		mNextFenceValue++;

		mFrameIndex = (mFrameIndex + 1) % mFramesInFlight;

		return std::async(std::launch::deferred, [this, currentFrameIndex]() { _WaitForFrame(currentFrameIndex); });
	}

	std::future<void> CopyQueue::UpdateSubresource(
		ID3D12Resource* pDstResource,
		ID3D12Resource* pIntermediateResource,
		UINT64 intermediateOffset,
		UINT firstSubresource,
		UINT numSubresources,
		D3D12_SUBRESOURCE_DATA* pSrcData,
		D3D12_RESOURCE_STATES state)
	{
		u8 currentFrameIndex = mFrameIndex;

		_WaitForFrame(currentFrameIndex);

		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandAllocator->Reset());
		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandList->Reset(mCopyFrames[currentFrameIndex].commandAllocator.p, nullptr));

		UpdateSubresources(mCopyFrames[currentFrameIndex].commandList.p, pDstResource, pIntermediateResource, intermediateOffset, firstSubresource, numSubresources, pSrcData);
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, D3D12_RESOURCE_STATE_COPY_DEST, state);
		mCopyFrames[currentFrameIndex].commandList->ResourceBarrier(1, &transition);

		THROW_IF_FAILED(mCopyFrames[currentFrameIndex].commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { mCopyFrames[currentFrameIndex].commandList.p };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		u64 fenceValue = mNextFenceValue;
		THROW_IF_FAILED(mCommandQueue->Signal(mFence.p, fenceValue));
		mCopyFrames[currentFrameIndex].fenceValue = fenceValue;
		mNextFenceValue++;

		mFrameIndex = (mFrameIndex + 1) % mFramesInFlight;

		return std::async(std::launch::deferred, [this, currentFrameIndex]() { _WaitForFrame(currentFrameIndex); });
	}

	void CopyQueue::Flush()
	{
		for (u8 i = 0; i < mFramesInFlight; ++i)
			_WaitForFrame(i);
	}

	void CopyQueue::_WaitForFrame(u8 frameIndex)
	{
		if (mFence->GetCompletedValue() < mCopyFrames[frameIndex].fenceValue)
		{
			THROW_IF_FAILED(mFence->SetEventOnCompletion(mCopyFrames[frameIndex].fenceValue, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}
	}

#pragma region Global Copy Queue

	static alignas(CopyQueue) std::aligned_storage_t<sizeof(CopyQueue), alignof(CopyQueue)> gGlobalCopyQueue;

	void InitializeGlobalCopyQueue()
	{
		std::construct_at(reinterpret_cast<CopyQueue*>(&gGlobalCopyQueue), GFX_DEFAULT_COPY_QUEUE_SIZE);
	}

	void ShutdownGlobalCopyQueue()
	{
		std::destroy_at(reinterpret_cast<CopyQueue*>(&gGlobalCopyQueue));
	}

	CopyQueue& GetEngineCopyQueue()
	{
		return *reinterpret_cast<CopyQueue*>(&gGlobalCopyQueue);
	}

	CComPtr<ID3D12Resource> CreateGPUBuffer(u32 bufferSize, void* data, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags)
	{
		CComPtr<ID3D12Resource> uploadBuffer;
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		THROW_IF_FAILED(gDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		));
		// populate the buffer
		void* mappedData;
		THROW_IF_FAILED(uploadBuffer->Map(0, nullptr, &mappedData));
		memcpy(mappedData, data, bufferSize);
		uploadBuffer->Unmap(0, nullptr);


		CComPtr<ID3D12Resource> gpuOnlyBuffer;
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		THROW_IF_FAILED(gDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&gpuOnlyBuffer)
		));

		D3D12_SUBRESOURCE_DATA subresourceData
		{
			.pData = data,
			.RowPitch = bufferSize,
			.SlicePitch = subresourceData.RowPitch
		};

		GetEngineCopyQueue().UpdateSubresource(gpuOnlyBuffer, uploadBuffer.p, 0, 0, 1, &subresourceData, D3D12_RESOURCE_STATE_COMMON).wait();

		return gpuOnlyBuffer;
	}


#pragma endregion
}