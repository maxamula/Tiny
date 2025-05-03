#include "gpuqueue.h"
#include "gfxglobal.h"

namespace tiny
{
	void D3DGpuQueue::Create(const D3D12_COMMAND_QUEUE_DESC& queueDesc, u8 bufferCount)
	{
		mCommandFrames.resize(bufferCount);
		mFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		mFramesInFlight = bufferCount;
		mFrameIndex = 0;
		mLastSubmittedFrame = 0;
		mNextFenceValue = 1;
		THROW_IF_FAILED(gDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
		for (u8 i = 0; i < bufferCount; ++i)
		{
			THROW_IF_FAILED(gDevice->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&mCommandFrames[i].commandAllocator)));
			THROW_IF_FAILED(gDevice->CreateCommandList(0, queueDesc.Type, mCommandFrames[i].commandAllocator.p, nullptr, IID_PPV_ARGS(&mCommandFrames[i].commandList)));
			THROW_IF_FAILED(mCommandFrames[i].commandList->Close());
			mCommandFrames[i].fenceValue = 0;
		}
		THROW_IF_FAILED(gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	}

	void D3DGpuQueue::Destroy()
	{
		mCommandFrames.clear();
		mCommandQueue.Release();
		mFence.Release();
		mCurrentCmdList.Release();
		if (mFenceEvent)
		{
			CloseHandle(mFenceEvent);
			mFenceEvent = nullptr;
		}
	}

	u8 D3DGpuQueue::BeginFrame()
	{
		_WaitForFrame(mFrameIndex);
		THROW_IF_FAILED(mCommandFrames[mFrameIndex].commandAllocator->Reset());
		THROW_IF_FAILED(mCommandFrames[mFrameIndex].commandList->Reset(mCommandFrames[mFrameIndex].commandAllocator.p, nullptr));
		mCurrentCmdList = mCommandFrames[mFrameIndex].commandList;
		return mFrameIndex;
	}

	std::future<void> D3DGpuQueue::EndFrame(std::function<void()> completionCallback)
	{
		THROW_IF_FAILED(mCurrentCmdList->Close());
		ID3D12CommandList* cmdLists[] = { mCurrentCmdList.p };
		mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

		if (completionCallback)
			completionCallback();

		mCommandFrames[mFrameIndex].fenceValue = mNextFenceValue++;
		THROW_IF_FAILED(mCommandQueue->Signal(mFence.p, mCommandFrames[mFrameIndex].fenceValue));

		mLastSubmittedFrame.store(mFrameIndex);
		mFrameIndex = (mFrameIndex + 1) % mFramesInFlight;

		return std::async(std::launch::deferred, [=]() { _WaitForFrame(mLastSubmittedFrame); });
	}

	void D3DGpuQueue::Flush()
	{
		for (u8 i = 0; i < mFramesInFlight; ++i)
			_WaitForFrame(i);
	}

	bool D3DGpuQueue::CheckFrameCompletion(u64 fence)
	{
		return mFence->GetCompletedValue() >= fence;
	}

	void D3DGpuQueue::_WaitForFrame(u8 frameIndex)
	{
		u64 fenceValue = mCommandFrames[frameIndex].fenceValue;
		if (fenceValue > 0 && mFence->GetCompletedValue() < fenceValue)
		{
			THROW_IF_FAILED(mFence->SetEventOnCompletion(fenceValue, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}
	}
}