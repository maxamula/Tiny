#include "framegraph.h"
#include "graphics/gfxglobal.h"
#include "d3dx12.hpp"
#include "pass.h"
#include <algorithm>
#include <cassert>

namespace tiny
{

    FrameGraph::FrameGraph()
    {
    }

    FrameGraph::~FrameGraph()
    {
    }

    FrameGraph::Builder::Builder(FrameGraph& frameGraph, int passIndex)
        : mFrameGraph(frameGraph)
        , mPassIndex(passIndex)
    {
    }

    void FrameGraph::Builder::Read(const RenderTextureHandle& handle)
    {
        uint32_t id = handle.Id;
        if (id == InvalidResourceId || id >= mFrameGraph.mRenderResourceInfos.size())
        {
            throw std::runtime_error("Invalid RenderTexture handle read");
        }
        auto& info = mFrameGraph.mRenderResourceInfos[id];
        if (info.LastWriterPassIndex == -1)
        {
            throw std::runtime_error("RenderTexture read with no producer");
        }
        mFrameGraph.AddDependency(info.LastWriterPassIndex, mPassIndex);
        info.ReaderPassIndices.push_back(mPassIndex);
        mFrameGraph.mPassNodes[mPassIndex].ReadsRender.push_back(handle);
    }

    void FrameGraph::Builder::Read(const DepthTextureHandle& handle)
    {
        uint32_t id = handle.Id;
        if (id == InvalidResourceId || id >= mFrameGraph.mDepthResourceInfos.size())
        {
            throw std::runtime_error("Invalid DepthTexture handle read");
        }
        auto& info = mFrameGraph.mDepthResourceInfos[id];
        if (info.LastWriterPassIndex == -1)
        {
            throw std::runtime_error("DepthTexture read with no producer");
        }
        mFrameGraph.AddDependency(info.LastWriterPassIndex, mPassIndex);
        info.ReaderPassIndices.push_back(mPassIndex);
        mFrameGraph.mPassNodes[mPassIndex].ReadsDepth.push_back(handle);
    }

    void FrameGraph::Builder::Write(const RenderTextureHandle& handle)
    {
        uint32_t id = handle.Id;
        if (id == InvalidResourceId || id >= mFrameGraph.mRenderResourceInfos.size())
        {
            throw std::runtime_error("Invalid RenderTexture handle write");
        }
        auto& info = mFrameGraph.mRenderResourceInfos[id];
        if (info.LastWriterPassIndex == mPassIndex)
        {
            if (!IsHandleInList(handle, mFrameGraph.mPassNodes[mPassIndex].WritesRender))
            {
                mFrameGraph.mPassNodes[mPassIndex].WritesRender.push_back(handle);
            }
            return;
        }
        if (info.LastWriterPassIndex != -1)
        {
            mFrameGraph.AddDependency(info.LastWriterPassIndex, mPassIndex);
            for (int r : info.ReaderPassIndices)
            {
                mFrameGraph.AddDependency(r, mPassIndex);
            }
        }
        info.LastWriterPassIndex = mPassIndex;
        if (!IsHandleInList(handle, mFrameGraph.mPassNodes[mPassIndex].WritesRender))
        {
            mFrameGraph.mPassNodes[mPassIndex].WritesRender.push_back(handle);
        }
    }

    void FrameGraph::Builder::Write(const DepthTextureHandle& handle)
    {
        uint32_t id = handle.Id;
        if (id == InvalidResourceId || id >= mFrameGraph.mDepthResourceInfos.size())
        {
            throw std::runtime_error("Invalid DepthTexture handle write");
        }
        auto& info = mFrameGraph.mDepthResourceInfos[id];
        if (info.LastWriterPassIndex == mPassIndex)
        {
            if (!IsHandleInList(handle, mFrameGraph.mPassNodes[mPassIndex].WritesDepth))
            {
                mFrameGraph.mPassNodes[mPassIndex].WritesDepth.push_back(handle);
            }
            return;
        }
        if (info.LastWriterPassIndex != -1)
        {
            mFrameGraph.AddDependency(info.LastWriterPassIndex, mPassIndex);
            for (int r : info.ReaderPassIndices)
            {
                mFrameGraph.AddDependency(r, mPassIndex);
            }
        }
        info.LastWriterPassIndex = mPassIndex;
        if (!IsHandleInList(handle, mFrameGraph.mPassNodes[mPassIndex].WritesDepth))
        {
            mFrameGraph.mPassNodes[mPassIndex].WritesDepth.push_back(handle);
        }
    }

    void FrameGraph::AddDependency(int src, int dst)
    {
        if (src < 0 || dst < 0 || src == dst)
        {
            return;
        }
        if (static_cast<int>(mDependencies.size()) <= src)
        {
            mDependencies.resize(src + 1);
        }
        auto& list = mDependencies[src];
        if (std::find(list.begin(), list.end(), dst) == list.end())
        {
            list.push_back(dst);
        }
    }

    void FrameGraph::Compile(D3DContext& d3d)
    {
        int passCount = static_cast<int>(mPassNodes.size());
        if (passCount == 0)
        {
            return;
        }
        std::vector<int> indegree(passCount);
        for (int i = 0; i < passCount; ++i)
        {
            for (int dst : mDependencies[i])
            {
                if (dst >= 0 && dst < passCount)
                {
                    ++indegree[dst];
                }
            }
        }

        std::deque<int> queue;
        for (int i = 0; i < passCount; ++i)
            if (indegree[i] == 0)
                queue.push_back(i);

        std::vector<int> sorted;
        sorted.reserve(passCount);

        while (!queue.empty())
        {
            int u = queue.front();
            queue.pop_front();
            sorted.push_back(u);
            for (int v : mDependencies[u])
                if (--indegree[v] == 0)
                    queue.push_back(v);
        }

        if (static_cast<int>(sorted.size()) != passCount)
        {
            throw std::runtime_error("Cycle detected in FrameGraph passes");
        }
        for (auto& res : mRenderResourceInfos)
        {
            res.FirstUseIndex = -1;
            res.LastUseIndex = -1;
        }
        for (auto& res : mDepthResourceInfos)
        {
            res.FirstUseIndex = -1;
            res.LastUseIndex = -1;
        }
        for (int order = 0; order < passCount; ++order)
        {
            int p = sorted[order];
            auto& node = mPassNodes[p];
            for (auto& h : node.ReadsRender)
            {
                auto& res = mRenderResourceInfos[h.Id];
                if (res.FirstUseIndex < 0) res.FirstUseIndex = order;
                res.LastUseIndex = std::max(res.LastUseIndex, order);
            }
            for (auto& h : node.WritesRender)
            {
                auto& res = mRenderResourceInfos[h.Id];
                if (res.FirstUseIndex < 0) res.FirstUseIndex = order;
                res.LastUseIndex = std::max(res.LastUseIndex, order);
            }
            for (auto& h : node.ReadsDepth)
            {
                auto& res = mDepthResourceInfos[h.Id];
                if (res.FirstUseIndex < 0) res.FirstUseIndex = order;
                res.LastUseIndex = std::max(res.LastUseIndex, order);
            }
            for (auto& h : node.WritesDepth)
            {
                auto& res = mDepthResourceInfos[h.Id];
                if (res.FirstUseIndex < 0) res.FirstUseIndex = order;
                res.LastUseIndex = std::max(res.LastUseIndex, order);
            }
        }
        auto allocatePool = [&](auto& resources, auto& allocations)
            {
                struct Entry { size_t Id; int First; int Last; };
                std::vector<Entry> list;
                for (size_t i = 0; i < resources.size(); ++i)
                {
                    if (resources[i].FirstUseIndex >= 0)
                    {
                        list.push_back({ i, resources[i].FirstUseIndex, resources[i].LastUseIndex });
                    }
                }
                std::sort(list.begin(), list.end(), [](auto& a, auto& b) { return a.First < b.First; });
                std::vector<size_t> active;
                for (auto& e : list)
                {
                    for (auto it = active.begin(); it != active.end();)
                    {
                        auto& act = resources[*it];
                        if (act.LastUseIndex < e.First)
                        {
                            allocations[act.PhysAllocationIndex].IsFree = true;
                            it = active.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    size_t needed = static_cast<size_t>(resources[e.Id].Desc.width) * resources[e.Id].Desc.height * 4U;
                    size_t chosen = SIZE_MAX;
                    size_t minWaste = SIZE_MAX;
                    for (size_t i = 0; i < allocations.size(); ++i)
                    {
                        if (allocations[i].IsFree && allocations[i].Size >= needed)
                        {
                            size_t waste = allocations[i].Size - needed;
                            if (waste < minWaste)
                            {
                                minWaste = waste;
                                chosen = i;
                            }
                        }
                    }
                    if (chosen == SIZE_MAX)
                    {
                        Allocation a{ needed, false, false };
                        chosen = allocations.size();
                        allocations.push_back(a);
                    }
                    else
                    {
                        allocations[chosen].IsFree = false;
                    }
                    resources[e.Id].PhysAllocationIndex = static_cast<int>(chosen);
                    active.push_back(e.Id);
                }
            };
        allocatePool(mRenderResourceInfos, mRenderAllocations);
        allocatePool(mDepthResourceInfos, mDepthAllocations);

        mRenderInstances.resize(mRenderAllocations.size());
        mDepthInstances.resize(mDepthAllocations.size());

        for (size_t i = 0; i < mRenderResourceInfos.size(); ++i)
        {
            auto& info = mRenderResourceInfos[i];
            int idx = info.PhysAllocationIndex;
            if (idx >= 0 && idx < (int)mRenderAllocations.size())
            {
                auto& alloc = mRenderAllocations[idx];
                if (!alloc.IsInitialized)
                {
                    mRenderInstances[idx] = RenderTexture::Create(info.Desc);
					SET_OBJECT_NAME(mRenderInstances[idx].resource, L"FrameGraph RT {}[{}]", i, idx);
                    d3d.Defer(mRenderInstances[idx].resource);
                    d3d.Defer(mRenderInstances[idx].rtv);
                    d3d.Defer(mRenderInstances[idx].srv);
                    alloc.IsInitialized = true;
                }
            }
        }

        for (size_t i = 0; i < mDepthResourceInfos.size(); ++i)
        {
            auto& info = mDepthResourceInfos[i];
            int idx = info.PhysAllocationIndex;
            if (idx >= 0 && idx < (int)mDepthAllocations.size())
            {
                auto& alloc = mDepthAllocations[idx];
                if (!alloc.IsInitialized)
                {
                    mDepthInstances[idx] = DepthTexture::Create(info.Desc);
                    SET_OBJECT_NAME(mDepthInstances[idx].resource, L"FrameGraph DT {}[{}]", i, idx);
                    d3d.Defer(mDepthInstances[idx].resource);
                    d3d.Defer(mDepthInstances[idx].dsv);
                    d3d.Defer(mDepthInstances[idx].srv);
                    alloc.IsInitialized = true;
                }
            }
        }

        mSortedPassIndices = sorted;

        mRenderStates.assign(mRenderAllocations.size(), D3D12_RESOURCE_STATE_COMMON);
        for (size_t i = 0; i < mRenderResourceInfos.size(); ++i)
        {
            int a = mRenderResourceInfos[i].PhysAllocationIndex;
            if (a >= 0) mRenderStates[a] = mRenderResourceInfos[i].InitialState;
        }
        mDepthStates.assign(mDepthAllocations.size(), D3D12_RESOURCE_STATE_COMMON);
        for (size_t i = 0; i < mDepthResourceInfos.size(); ++i)
        {
            int a = mDepthResourceInfos[i].PhysAllocationIndex;
            if (a >= 0) mDepthStates[a] = mDepthResourceInfos[i].InitialState;
        }

        D3D12_QUERY_HEAP_DESC queryHeapDesc
        {
            .Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
            .Count = 2,
            .NodeMask = 0
        };

        mFrameTimingData.timestampHeap.Release();
        mFrameTimingData.timestampResult.Release();
        THROW_IF_FAILED(gDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&mFrameTimingData.timestampHeap)));

        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(u64));
        CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        THROW_IF_FAILED(gDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mFrameTimingData.timestampResult)
        ));
    }

    void MakeContiguousBatches(const std::vector<int>& sortedPassIndices, const std::vector<double>& costs, u32 batchCount, std::vector<std::vector<u32>>& batches)
    {
        u32 N = sortedPassIndices.size();
        f64 total = std::accumulate(costs.begin(), costs.end(), 0.0);
        f64 target = total / batchCount;

        batches.reserve(batchCount);

        u32 start = 0;
        for (int b = 0; b < batchCount; ++b)
        {
            f64 acc = 0;
            u32 end = start;
            while (end < N && (acc + costs[end] <= target || end == start))
            {
                acc += costs[end++];
            }
            batches.emplace_back(sortedPassIndices.begin() + start, sortedPassIndices.begin() + end);
            start = end;
        }
        if (start < N)
        {
            auto& last = batches.back();
            last.insert(last.end(), sortedPassIndices.begin() + start, sortedPassIndices.end());
        }
    }

    void FrameGraph::ExecuteAsync(tf::Subflow& sf,
        D3DContext& d3d,
        RenderView& view,
        ID3D12Resource* pBackBuf)
    {
        if (mSortedPassIndices.empty() && !mPassNodes.empty())
            THROW_ENGINE_EXCEPTION("FrameGraph not compiled. Call Compile() before Execute().");

        FrameGraphResources resources;
        resources.mRenderTextures = &mRenderInstances;
        resources.mDepthTextures = &mDepthInstances;
        resources.mRenderResourceInfos = &mRenderResourceInfos;
        resources.mDepthResourceInfos = &mDepthResourceInfos;

        std::vector<f64> costs;
        costs.reserve(mSortedPassIndices.size());
        for (i32 idx : mSortedPassIndices)
            costs.push_back(mPassNodes[idx].Pass->EstimateCost());

        f64 sumCost = std::accumulate(costs.begin(), costs.end(), 0.0);
        u32 maxLists = TINY_COMMAND_LIST_POOL_SIZE;
        u32 desiredBatches = std::clamp(u32(sumCost / 500.0 + 0.5), 1u, maxLists);

        std::vector<std::vector<u32>> batches;
        MakeContiguousBatches(mSortedPassIndices, costs, desiredBatches, batches);

        std::vector<CComPtr<ID3D12GraphicsCommandList6>> batchCmds(batches.size());
        for (u32 i = 0; i < batches.size(); ++i)
            batchCmds[i] = d3d.mainQueue.AcquireNextCommandList();

        mFrameBarriers.assign(mPassNodes.size(), {});
        std::vector<D3D12_RESOURCE_STATES> curR = mRenderStates; // copy
        std::vector<D3D12_RESOURCE_STATES> curD = mDepthStates;

        auto wantRT = [](bool w) {return w ? D3D12_RESOURCE_STATE_RENDER_TARGET :
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;};
        auto wantDS = [](bool w) {return w ? D3D12_RESOURCE_STATE_DEPTH_WRITE :
            D3D12_RESOURCE_STATE_DEPTH_READ;};

        for (int p : mSortedPassIndices) {
            auto& node = mPassNodes[p];
            auto& out = mFrameBarriers[p];
            for (auto h : node.ReadsRender) {
                int a = mRenderResourceInfos[h.Id].PhysAllocationIndex;
                auto need = wantRT(false);if (curR[a] != need) { out.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(mRenderInstances[a].resource, curR[a], need));curR[a] = need; }
            }
            for (auto h : node.WritesRender) {
                int a = mRenderResourceInfos[h.Id].PhysAllocationIndex;
                auto need = wantRT(true); if (curR[a] != need) { out.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(mRenderInstances[a].resource, curR[a], need));curR[a] = need; }
            }
            for (auto h : node.ReadsDepth) {
                int a = mDepthResourceInfos[h.Id].PhysAllocationIndex;
                auto need = wantDS(false);if (curD[a] != need) { out.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(mDepthInstances[a].resource, curD[a], need));curD[a] = need; }
            }
            for (auto h : node.WritesDepth) {
                int a = mDepthResourceInfos[h.Id].PhysAllocationIndex;
                auto need = wantDS(true); if (curD[a] != need) { out.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(mDepthInstances[a].resource, curD[a], need));curD[a] = need; }
            }
        }

        auto recordBatch = [&](u32 batchIdx)
        {
             auto& cmd = batchCmds[batchIdx];

             ID3D12DescriptorHeap* heaps[] = {
                    *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
                    *GetEngineDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
                cmd->SetDescriptorHeaps(_countof(heaps), heaps);

                if (batchIdx == 0)
                {
                    cmd->EndQuery(mFrameTimingData.timestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
                    CD3DX12_RESOURCE_BARRIER bb = CD3DX12_RESOURCE_BARRIER::Transition(
                        pBackBuf, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    cmd->ResourceBarrier(1, &bb);
                }

                for (u32 passIdx : batches[batchIdx])
                {
                    PassNode& node = mPassNodes[passIdx];
                    if (!mFrameBarriers[passIdx].empty())
                        cmd->ResourceBarrier(static_cast<u32>(mFrameBarriers[passIdx].size()), mFrameBarriers[passIdx].data());

                    RenderContext ctx
                    {
                        .cmd = cmd,
						.renderView = view
                    };
                    node.Pass->ExecuteRecordElapsed(ctx, resources);
                }


                if (batchIdx == batches.size() - 1)
                {
                    cmd->EndQuery(mFrameTimingData.timestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
                    cmd->ResolveQueryData(mFrameTimingData.timestampHeap,
                        D3D12_QUERY_TYPE_TIMESTAMP, 0, 2,
                        mFrameTimingData.timestampResult, 0);
                    CD3DX12_RESOURCE_BARRIER bb = CD3DX12_RESOURCE_BARRIER::Transition(
                        pBackBuf, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
                    cmd->ResourceBarrier(1, &bb);
                }
            };

        for (u32 i = 0; i < batches.size(); ++i)
            sf.emplace([&, i]() { recordBatch(i); });

        sf.join();
        mRenderStates.swap(curR);
        mDepthStates.swap(curD);
    }

    RenderTexture& FrameGraphResources::GetRenderTexture(const RenderTextureHandle& handle)
    {
        assert(handle.Id != InvalidResourceId && handle.Id < mRenderResourceInfos->size());
        const auto& info = (*mRenderResourceInfos)[handle.Id];
        assert(info.PhysAllocationIndex >= 0 && info.PhysAllocationIndex < static_cast<int>(mRenderTextures->size()));
        return (*mRenderTextures)[info.PhysAllocationIndex];
    }

    DepthTexture& FrameGraphResources::GetDepthTexture(const DepthTextureHandle& handle)
    {
        assert(handle.Id != InvalidResourceId && handle.Id < mDepthResourceInfos->size());
        const auto& info = (*mDepthResourceInfos)[handle.Id];
        assert(info.PhysAllocationIndex >= 0 && info.PhysAllocationIndex < static_cast<int>(mDepthTextures->size()));
        return (*mDepthTextures)[info.PhysAllocationIndex];
    }

    RenderTextureHandle FrameGraph::CreateRenderTexture(const RenderTexture::Desc& desc)
    {
        uint32_t id = static_cast<uint32_t>(mRenderResourceInfos.size());
        RenderResourceInfo info{ desc, -1, -1, {}, -1, -1, -1 };
        mRenderResourceInfos.push_back(info);
        return RenderTextureHandle(id);
    }

    DepthTextureHandle FrameGraph::CreateDepthTexture(const DepthTexture::Desc& desc)
    {
        uint32_t id = static_cast<uint32_t>(mDepthResourceInfos.size());
        DepthResourceInfo info{ desc, -1, -1, {}, -1, -1, -1 };
        mDepthResourceInfos.push_back(info);
        return DepthTextureHandle(id);
    }

    void FrameGraph::Reset()
    {
		mRenderResourceInfos.clear();
		mDepthResourceInfos.clear();
		mRenderAllocations.clear();
		mDepthAllocations.clear();
		mRenderInstances.clear();
		mDepthInstances.clear();
		mPassNodes.clear();
		mDependencies.clear();
		mSortedPassIndices.clear();
    }
}