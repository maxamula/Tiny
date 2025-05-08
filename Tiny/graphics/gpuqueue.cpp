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
			for (auto& ctx : mCommandFrames[i].commandListContexts)
			{
				THROW_IF_FAILED(gDevice->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&ctx.commandAllocator)));
				THROW_IF_FAILED(gDevice->CreateCommandList(0, queueDesc.Type, ctx.commandAllocator.p, nullptr, IID_PPV_ARGS(&ctx.commandList)));
				ctx.commandList->Close();
			}
			mCommandFrames[i].allocatedCommandListCount = 0;
			mCommandFrames[i].fenceValue = 0;
		}
		THROW_IF_FAILED(gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	}

	void D3DGpuQueue::Destroy()
	{
		mCommandFrames.clear();
		mCommandQueue.Release();
		mFence.Release();
		if (mFenceEvent)
		{
			CloseHandle(mFenceEvent);
			mFenceEvent = nullptr;
		}
	}

	u8 D3DGpuQueue::BeginFrame()
	{
		_WaitForFrame(mFrameIndex);
		return mFrameIndex;
	}

	void D3DGpuQueue::EndFrame(std::function<void()> completionCallback)
	{
		std::vector<ID3D12CommandList*> commandLists;
		commandLists.reserve(mCommandFrames[mFrameIndex].allocatedCommandListCount);
		for (u8 i = 0; i < mCommandFrames[mFrameIndex].allocatedCommandListCount; ++i)
		{
			auto& ctx = mCommandFrames[mFrameIndex].commandListContexts[i];
			THROW_IF_FAILED(ctx.commandList->Close());
			commandLists.push_back(ctx.commandList.p);
		}
		mCommandQueue->ExecuteCommandLists(mCommandFrames[mFrameIndex].allocatedCommandListCount, commandLists.data());
		mCommandFrames[mFrameIndex].allocatedCommandListCount = 0;
		
		if (completionCallback)
			completionCallback();

		mCommandFrames[mFrameIndex].fenceValue = mNextFenceValue++;
		THROW_IF_FAILED(mCommandQueue->Signal(mFence.p, mCommandFrames[mFrameIndex].fenceValue));

		mLastSubmittedFrame.store(mFrameIndex);
		mFrameIndex = (mFrameIndex + 1) % mFramesInFlight;
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

	CComPtr<ID3D12GraphicsCommandList6> D3DGpuQueue::AcquireNextCommandList()
	{
		u8 frameIndex = mFrameIndex;
		auto& ctx = mCommandFrames[frameIndex].commandListContexts[mCommandFrames[frameIndex].allocatedCommandListCount++];
		THROW_IF_FAILED(ctx.commandAllocator->Reset());
		THROW_IF_FAILED(ctx.commandList->Reset(ctx.commandAllocator.p, nullptr));
		return ctx.commandList;
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