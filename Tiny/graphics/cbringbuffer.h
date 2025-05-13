#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <deque>
#include <mutex>
#include <cassert>
#include <windows.h>


namespace tiny
{
    class FrameRingBuffer
    {
    public:
        struct Allocation
        {
            D3D12_GPU_VIRTUAL_ADDRESS   gpuAddress;
            void*                       cpuAddress;
            size_t                      offset;
            size_t                      size;
        };

        FrameRingBuffer() = default;
        ~FrameRingBuffer();
        void Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, size_t bufferSize);
        void Destroy();
        Allocation Allocate(size_t size);
    private:
        struct Pending
        {
            size_t offset;
            size_t size;
            UINT64 fenceValue;
        };

        CComPtr<ID3D12Resource> mBuffer;
        UINT8*                  mMappedData = nullptr;
        size_t                  mBufferSize = 0;
        size_t                  mHead = 0;

        std::deque<Pending>     mPending;
        CComPtr<ID3D12Fence>    mFence;
        UINT64                  mFenceValue = 0;
        HANDLE                  mFenceEvent = nullptr;
        ID3D12CommandQueue*     mQueue = nullptr;
        std::mutex              mMutex;

        void DiscardCompleted();
    };
}