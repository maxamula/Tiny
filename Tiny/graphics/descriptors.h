#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <map>
#include <mutex>
#include <unordered_map>

namespace tiny
{
	class DescriptorHandle;
	class DescriptorRange;
	class D3DDescriptorHeap;

	class TINYFX_API DescriptorHandle final
	{
		friend class D3DDescriptorHeap;
	public:
		DescriptorHandle();
		DescriptorHandle(const DescriptorHandle&) noexcept;
		DescriptorHandle& operator=(const DescriptorHandle&) noexcept;
		DescriptorHandle(DescriptorHandle&&) noexcept;
		DescriptorHandle& operator=(DescriptorHandle&&) noexcept;
		~DescriptorHandle();

		void Release();

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept;
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept;

		__declspec(property(get = GetCpuHandle)) D3D12_CPU_DESCRIPTOR_HANDLE cpu;
		__declspec(property(get = GetGpuHandle)) D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	private:
		DescriptorHandle(D3DDescriptorHeap* heap, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

		D3DDescriptorHeap* mHeap;
		u32								mIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE		mCpu;
		D3D12_GPU_DESCRIPTOR_HANDLE		mGpu;
	};

	class TINYFX_API DescriptorRange final
	{
		friend class D3DDescriptorHeap;
	public:
		DescriptorRange();
		DescriptorRange(const DescriptorRange&) noexcept;
		DescriptorRange& operator=(const DescriptorRange&) noexcept;
		DescriptorRange(DescriptorRange&&) noexcept;
		DescriptorRange& operator=(DescriptorRange&&) noexcept;
		~DescriptorRange();

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(int index) const;
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(int index) const;

		__declspec(property(get = GetCpuHandle)) D3D12_CPU_DESCRIPTOR_HANDLE cpu[];
		__declspec(property(get = GetGpuHandle)) D3D12_GPU_DESCRIPTOR_HANDLE gpu[];
	private:
		DescriptorRange(D3DDescriptorHeap* heap, u32 index, u32 length, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

		D3DDescriptorHeap* mHeap;
		u32							mIndex;
		u32							mLength;
		D3D12_CPU_DESCRIPTOR_HANDLE	mSartCpu;
		D3D12_GPU_DESCRIPTOR_HANDLE	mStartGpu;
	};

	class TINYFX_API D3DDescriptorHeap final
	{
		struct AllocationInfo
		{
			u32 refCount = 0;
			u32 length = 0;
		};
	public:
		D3DDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptors);
		D3DDescriptorHeap(const D3DDescriptorHeap&) = delete;
		D3DDescriptorHeap& operator=(const D3DDescriptorHeap&) = delete;
		D3DDescriptorHeap(D3DDescriptorHeap&&) = delete;
		D3DDescriptorHeap& operator=(D3DDescriptorHeap&&) = delete;
		~D3DDescriptorHeap() = default;
		[[nodiscard]] inline bool IsShaderVisible() const noexcept { return mShaderVisible; }
		[[nodiscard]] inline D3D12_DESCRIPTOR_HEAP_TYPE GetType() const noexcept { return mType; }
		[[nodiscard]] DescriptorHandle AllocateDescriptor();
		[[nodiscard]] DescriptorRange AllocateRange(u32 length);
		[[nodiscard]] ID3D12DescriptorHeap* operator*() const noexcept { return mDescriptorHeap; }
		void AddRef(u32 index);
		void Release(u32 index);
		void Release(u32 index, u32 len);
	private:
		[[nodiscard]] u32 AllocateInternal(u32 length);
		void ReleaseInternal(u32 startIndex, u32 length);
		void MergeFreeRanges(u32 freedStart, u32 freedLength);

		D3D12_DESCRIPTOR_HEAP_TYPE				mType;
		u32										mNumDescriptors;
		u32										mDescriptorSize;
		bool									mShaderVisible;
		CComPtr<ID3D12DescriptorHeap>			mDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE				mCpuStart;
		D3D12_GPU_DESCRIPTOR_HANDLE				mGpuStart;
		std::map<u32, u32>						mFreeRanges;
		std::unordered_map<u32, AllocationInfo> mAllocations;
		std::mutex								mMutex;
	};

	void InitializeGlobalDescriptorHeaps();
	void ShutdownGlobalDescriptorHeaps();
	TINYFX_API [[nodiscard]] D3DDescriptorHeap& GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
}