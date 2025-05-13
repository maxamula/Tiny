#include "cbringbuffer.h"

namespace tiny
{
    static size_t Align256(size_t x)
    {
        return (x + 255) & ~255;
    }

    FrameRingBuffer::~FrameRingBuffer()
    {
        Destroy();
    }

    void FrameRingBuffer::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, size_t bufferSize)
    {
        assert(bufferSize % 256 == 0);
        mQueue = queue;
        mBufferSize = bufferSize;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = bufferSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mBuffer));
        assert(SUCCEEDED(hr));

        hr = mBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
        assert(SUCCEEDED(hr));

        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
        assert(SUCCEEDED(hr));
        mFenceValue = 0;
        mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(mFenceEvent);
    }

    void FrameRingBuffer::Destroy()
    {
        if (mMappedData)
        {
            mBuffer->Unmap(0, nullptr);
            mMappedData = nullptr;
        }
        if (mFenceEvent)
        {
            CloseHandle(mFenceEvent);
            mFenceEvent = nullptr;
        }
        mBuffer.Release();
        mFence.Release();
    }

    void FrameRingBuffer::DiscardCompleted()
    {
        UINT64 completed = mFence->GetCompletedValue();
        while (!mPending.empty() && mPending.front().fenceValue <= completed)
        {
            mPending.pop_front();
        }
    }

    FrameRingBuffer::Allocation FrameRingBuffer::Allocate(size_t size)
    {
        size_t aligned = Align256(size);
        assert(aligned <= mBufferSize);

        std::unique_lock<std::mutex> lock(mMutex);

        size_t allocOffset = mHead;

        while (true)
        {
            DiscardCompleted();

            if (allocOffset + aligned > mBufferSize)
            {
                allocOffset = 0;
                DiscardCompleted();
            }

            bool collision = false;
            if (!mPending.empty()) {
                size_t tail = mPending.front().offset;
                if (allocOffset < tail)
                    collision = (allocOffset + aligned > tail);
                else
                {
                    if (allocOffset + aligned > mBufferSize)
                        collision = (aligned > tail);
                }
            }

            if (!collision)
                break;

            UINT64 waitFor = mPending.front().fenceValue;
            mFence->SetEventOnCompletion(waitFor, mFenceEvent);
            WaitForSingleObject(mFenceEvent, INFINITE);
        }

        mHead = allocOffset + aligned;
        if (mHead >= mBufferSize)
            mHead = 0;

        UINT64 thisFence = ++mFenceValue;
        mQueue->Signal(mFence, thisFence);

        mPending.push_back({ allocOffset, aligned, thisFence });

        return
        {
            mBuffer->GetGPUVirtualAddress() + allocOffset,
            mMappedData + allocOffset,
            allocOffset,
            aligned
        };
    }
}