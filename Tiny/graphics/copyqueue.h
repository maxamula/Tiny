#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <vector>
#include <future>

namespace tiny
{
	class TINYFX_API CopyQueue
	{
	public:
		CopyQueue(u8 bufferCount);
		CopyQueue(const CopyQueue&) = delete;
		CopyQueue(CopyQueue&&) = delete;
		CopyQueue& operator=(const CopyQueue&) = delete;
		CopyQueue& operator=(CopyQueue&&) = delete;
		~CopyQueue() = default;

		std::future<void> CopyResource(ID3D12Resource* pDstResource, ID3D12Resource* pSrcResource);
		std::future<void> UpdateSubresource(ID3D12Resource* pDstResource, ID3D12Resource* pIntermediateResource,
			UINT64 intermediateOffset, UINT firstSubresource, UINT numSubresources, D3D12_SUBRESOURCE_DATA* pSrcData, D3D12_RESOURCE_STATES state);
		void Flush();

		[[nodiscard]] inline ID3D12CommandQueue* operator*() const { return mCommandQueue.p; }
	private:
		struct COPY_FRAME
		{
			COPY_FRAME()
				: commandAllocator(nullptr), commandList(nullptr), fenceValue(0)
			{}

			CComPtr<ID3D12CommandAllocator>			commandAllocator;
			CComPtr<ID3D12GraphicsCommandList>		commandList;
			u64										fenceValue;
		};

		void _WaitForFrame(u8 frameIndex);

		CComPtr<ID3D12CommandQueue>         mCommandQueue;
		std::vector<COPY_FRAME>             mCopyFrames;
		CComPtr<ID3D12Fence>                mFence;
		HANDLE								mFenceEvent;
		u8									mFrameIndex;
		u64									mNextFenceValue;
		u8									mFramesInFlight;
	};

	void InitializeGlobalCopyQueue();
	void ShutdownGlobalCopyQueue();
	TINY_API CopyQueue& GetEngineCopyQueue();
	TINY_API CComPtr<ID3D12Resource> CreateGPUBuffer(u32 bufferSize, void* data, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
}