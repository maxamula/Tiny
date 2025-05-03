#include "descriptors.h"
#include "gfxglobal.h"

namespace tiny
{
	D3DDescriptorHeap::D3DDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptors)
		: mType(type), mNumDescriptors(numDescriptors), mDescriptorSize(gDevice->GetDescriptorHandleIncrementSize(type)),
		mShaderVisible(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc
		{
			.Type = type,
			.NumDescriptors = numDescriptors,
			.Flags = mShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		THROW_IF_FAILED(gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
		mCpuStart = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		if (mShaderVisible)
			mGpuStart = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		mFreeRanges[0] = numDescriptors;
	}

	DescriptorHandle D3DDescriptorHeap::AllocateDescriptor()
	{
		u32 startIndex = AllocateInternal(1);

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCpuStart;
		cpuHandle.ptr += static_cast<SIZE_T>(startIndex) * mDescriptorSize;

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
		if (mShaderVisible)
		{
			gpuHandle = mGpuStart;
			gpuHandle.ptr += static_cast<UINT64>(startIndex) * mDescriptorSize;
		}

		return DescriptorHandle(this, startIndex, cpuHandle, gpuHandle);
	}

	DescriptorRange D3DDescriptorHeap::AllocateRange(u32 length)
	{
		u32 startIndex = AllocateInternal(length);

		D3D12_CPU_DESCRIPTOR_HANDLE startCpu = mCpuStart;
		startCpu.ptr += static_cast<SIZE_T>(startIndex) * mDescriptorSize;

		D3D12_GPU_DESCRIPTOR_HANDLE startGpu = {};
		if (mShaderVisible)
		{
			startGpu = mGpuStart;
			startGpu.ptr += static_cast<UINT64>(startIndex) * mDescriptorSize;
		}

		return DescriptorRange(this, startIndex, length, startCpu, startGpu);
	}

	u32 D3DDescriptorHeap::AllocateInternal(u32 length)
	{
		std::lock_guard lock(mMutex);

		for (auto it = mFreeRanges.begin(); it != mFreeRanges.end(); ++it)
		{
			u32 freeStart = it->first;
			u32 freeLength = it->second;

			if (freeLength >= length)
			{
				u32 allocatedStart = freeStart;

				u32 remainingLength = freeLength - length;
				mFreeRanges.erase(it);

				if (remainingLength > 0)
				{
					u32 newStart = freeStart + length;
					mFreeRanges[newStart] = remainingLength;
				}

				AllocationInfo info;
				info.refCount = 0;
				info.length = length;
				mAllocations[allocatedStart] = info;
				return allocatedStart;
			}
		}

		THROW_ENGINE_EXCEPTION("Out of descriptors in heap");
	}

	void D3DDescriptorHeap::AddRef(u32 index)
	{
		std::lock_guard lock(mMutex);
		auto it = mAllocations.find(index);
		if (it == mAllocations.end())
			THROW_ENGINE_EXCEPTION("AddRef - Invalid index");
		it->second.refCount++;
	}

	void D3DDescriptorHeap::Release(u32 index)
	{
		ReleaseInternal(index, 1);
	}

	void D3DDescriptorHeap::Release(u32 index, u32 len)
	{
		ReleaseInternal(index, len);
	}

	void D3DDescriptorHeap::ReleaseInternal(u32 startIndex, u32 length)
	{
		std::lock_guard lock(mMutex);

		auto allocIt = mAllocations.find(startIndex);
		if (allocIt == mAllocations.end())
			THROW_ENGINE_EXCEPTION("Release - Invalid index");

		AllocationInfo& info = allocIt->second;
		if (info.refCount > 0)
			--info.refCount;

		if (info.refCount > 0)
			return;

		mAllocations.erase(allocIt);
		MergeFreeRanges(startIndex, length);
	}

	void D3DDescriptorHeap::MergeFreeRanges(u32 freedStart, u32 freedLength)
	{
		auto it = mFreeRanges.lower_bound(freedStart);

		if (it != mFreeRanges.begin())
		{
			auto prev = std::prev(it);
			u32 prevStart = prev->first;
			u32 prevLength = prev->second;
			u32 prevEnd = prevStart + prevLength;

			if (prevEnd == freedStart)
			{
				freedStart = prevStart;
				freedLength += prevLength;
				mFreeRanges.erase(prev);
			}
		}

		auto inserted = mFreeRanges.emplace(freedStart, freedLength).first;

		auto next = std::next(inserted);
		if (next != mFreeRanges.end())
		{
			u32 nextStart = next->first;
			u32 nextLength = next->second;
			u32 insertedEnd = freedStart + freedLength;

			if (insertedEnd == nextStart)
			{
				inserted->second += nextLength;
				mFreeRanges.erase(next);
			}
		}
	}

#pragma region DescriptorHandle

	DescriptorHandle::DescriptorHandle()
		: mHeap(nullptr), mIndex(0), mCpu({ 0 }), mGpu({ 0 })
	{
	}

	DescriptorHandle::DescriptorHandle(D3DDescriptorHeap* heap, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
		: mHeap(heap), mIndex(index), mCpu(cpu), mGpu(gpu)
	{
		if (mHeap)
			mHeap->AddRef(mIndex);
	}

	DescriptorHandle::DescriptorHandle(const DescriptorHandle& other) noexcept
		: mHeap(other.mHeap), mIndex(other.mIndex), mCpu(other.mCpu), mGpu(other.mGpu)
	{
		if (mHeap)
			mHeap->AddRef(mIndex);
	}

	DescriptorHandle& DescriptorHandle::operator=(const DescriptorHandle& other) noexcept
	{
		if (this != &other)
		{
			if (mHeap)
				mHeap->Release(mIndex);

			mHeap = other.mHeap;
			mIndex = other.mIndex;
			mCpu = other.mCpu;
			mGpu = other.mGpu;

			if (mHeap)
				mHeap->AddRef(mIndex);
		}
		return *this;
	}

	DescriptorHandle::DescriptorHandle(DescriptorHandle&& other) noexcept
		: mHeap(other.mHeap), mIndex(other.mIndex), mCpu(other.mCpu), mGpu(other.mGpu)
	{
		other.mHeap = nullptr;
		other.mIndex = 0;
		other.mCpu = { 0 };
		other.mGpu = { 0 };
	}

	DescriptorHandle& DescriptorHandle::operator=(DescriptorHandle&& other) noexcept
	{
		if (this != &other)
		{
			if (mHeap)
				mHeap->Release(mIndex);

			mHeap = other.mHeap;
			mIndex = other.mIndex;
			mCpu = other.mCpu;
			mGpu = other.mGpu;

			other.mHeap = nullptr;
			other.mIndex = 0;
			other.mCpu = { 0 };
			other.mGpu = { 0 };
		}
		return *this;
	}

	DescriptorHandle::~DescriptorHandle()
	{
		Release();
	}

	void DescriptorHandle::Release()
	{
		if (mHeap)
			mHeap->Release(mIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::GetCpuHandle() const noexcept
	{
		return mCpu;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::GetGpuHandle() const noexcept
	{
		return mGpu;
	}

#pragma endregion

#pragma region DescriptorRange

	DescriptorRange::DescriptorRange()
		: mHeap(nullptr), mIndex(0), mLength(0), mSartCpu({ 0 }), mStartGpu({ 0 })
	{
	}

	DescriptorRange::DescriptorRange(D3DDescriptorHeap* heap, u32 index, u32 length, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
		: mHeap(heap), mIndex(index), mLength(length), mSartCpu(cpu), mStartGpu(gpu)
	{
	}

	DescriptorRange::DescriptorRange(const DescriptorRange& other) noexcept
		: mHeap(other.mHeap), mIndex(other.mIndex), mLength(other.mLength), mSartCpu(other.mSartCpu), mStartGpu(other.mStartGpu)
	{
		if (mHeap)
			mHeap->AddRef(mIndex);
	}

	DescriptorRange& DescriptorRange::operator=(const DescriptorRange& other) noexcept
	{
		if (this != &other)
		{
			if (mHeap)
				mHeap->Release(mIndex, mLength);

			mHeap = other.mHeap;
			mIndex = other.mIndex;
			mLength = other.mLength;
			mSartCpu = other.mSartCpu;
			mStartGpu = other.mStartGpu;

			if (mHeap)
				mHeap->AddRef(mIndex);
		}
		return *this;
	}

	DescriptorRange::DescriptorRange(DescriptorRange&& other) noexcept
		: mHeap(other.mHeap), mIndex(other.mIndex), mLength(other.mLength), mSartCpu(other.mSartCpu), mStartGpu(other.mStartGpu)
	{
		other.mHeap = nullptr;
		other.mIndex = 0;
		other.mLength = 0;
		other.mSartCpu = { 0 };
		other.mStartGpu = { 0 };
	}

	DescriptorRange& DescriptorRange::operator=(DescriptorRange&& other) noexcept
	{
		if (this != &other)
		{
			if (mHeap)
				mHeap->Release(mIndex, mLength);

			mHeap = other.mHeap;
			mIndex = other.mIndex;
			mLength = other.mLength;
			mSartCpu = other.mSartCpu;
			mStartGpu = other.mStartGpu;

			other.mHeap = nullptr;
			other.mIndex = 0;
			other.mLength = 0;
			other.mSartCpu = { 0 };
			other.mStartGpu = { 0 };
		}
		return *this;
	}

	DescriptorRange::~DescriptorRange()
	{
		if (mHeap)
			mHeap->Release(mIndex, mLength);
		ZeroMemory(this, sizeof(*this));
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorRange::GetCpuHandle(int index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = mSartCpu;
		cpu.ptr += index * gDevice->GetDescriptorHandleIncrementSize(mHeap->GetType());
		return cpu;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorRange::GetGpuHandle(int index) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpu = mStartGpu;
		gpu.ptr += index * gDevice->GetDescriptorHandleIncrementSize(mHeap->GetType());
		return gpu;
	}

	static alignas(D3DDescriptorHeap) std::aligned_storage_t<sizeof(D3DDescriptorHeap), alignof(D3DDescriptorHeap)> gGlobalDescriptorHeaps[4];

	void InitializeGlobalDescriptorHeaps()
	{
		std::construct_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GFX_DEFAULT_SRV_HEAP_SIZE);
		std::construct_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GFX_DEFAULT_SAMPLER_HEAP_SIZE);
		std::construct_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, GFX_DEFAULT_RTV_HEAP_SIZE);
		std::construct_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, GFX_DEFAULT_DSV_HEAP_SIZE);
	}

	void ShutdownGlobalDescriptorHeaps()
	{
		std::destroy_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]));
		std::destroy_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]));
		std::destroy_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]));
		std::destroy_at(reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]));
	}

	D3DDescriptorHeap& GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
	{
		return *reinterpret_cast<D3DDescriptorHeap*>(&gGlobalDescriptorHeaps[type]);
	}
}