#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <future>

namespace tiny
{
	class D3DGpuQueue final
	{
	public:
		D3DGpuQueue() = default;
		~D3DGpuQueue() = default;
		D3DGpuQueue(const D3DGpuQueue&) = delete;
		D3DGpuQueue& operator=(const D3DGpuQueue&) = delete;

		void Create(const D3D12_COMMAND_QUEUE_DESC& queueDesc, u8 bufferCount);
		void Destroy();

		u8 BeginFrame();
		std::future<void> EndFrame(std::function<void()> completionCallback);
		void Flush();
		bool CheckFrameCompletion(u64 fence);

		// Getters
		ID3D12CommandQueue* GetQueue() const { return mCommandQueue.p; }
		ID3D12GraphicsCommandList6* GetCurrentCommandList() const { return mCurrentCmdList.p; }
		u8 GetFrameIndex() const { return mFrameIndex.load(); }
		u64 GetNextFenceValue() const { return mNextFenceValue.load(); }

	private:
		struct CommandFrame
		{
			CComPtr<ID3D12CommandAllocator>		commandAllocator;
			CComPtr<ID3D12GraphicsCommandList6>	commandList;
			u64									fenceValue;
		};

		void _WaitForFrame(u8 frameIndex);

		CComPtr<ID3D12CommandQueue>			mCommandQueue;
		std::vector<CommandFrame>			mCommandFrames;
		CComPtr<ID3D12GraphicsCommandList6>	mCurrentCmdList;
		CComPtr<ID3D12Fence>				mFence;
		HANDLE								mFenceEvent;
		std::atomic<u8>						mLastSubmittedFrame;
		std::atomic<u8>						mFrameIndex;
		std::atomic<u64>					mNextFenceValue;
		u8									mFramesInFlight;
	};
}