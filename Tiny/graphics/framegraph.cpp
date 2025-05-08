#include "framegraph.h"
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
        std::vector<int> stack;
        for (int i = 0; i < passCount; ++i)
        {
            if (indegree[i] == 0)
            {
                stack.push_back(i);
            }
        }
        std::vector<int> sorted;
        sorted.reserve(passCount);
        while (!stack.empty())
        {
            int u = stack.back();
            stack.pop_back();
            sorted.push_back(u);
            for (int v : mDependencies[u])
            {
                if (--indegree[v] == 0)
                {
                    stack.push_back(v);
                }
            }
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
                    d3d.Defer(mDepthInstances[idx].resource);
                    d3d.Defer(mDepthInstances[idx].dsv);
                    d3d.Defer(mDepthInstances[idx].srv);
                    alloc.IsInitialized = true;
                }
            }
        }

        mSortedPassIndices = sorted;
    }

    void FrameGraph::Execute(tf::Subflow& sf, RenderContext& ctx, D3DContext& d3d)
    {
        if (mSortedPassIndices.empty() && !mPassNodes.empty())
			THROW_ENGINE_EXCEPTION("FrameGraph not compiled. Call Compile() before Execute().");

        // Track last barrier state per allocation
        std::vector<ResourceState> renderStates(mRenderAllocations.size(), ResourceState::Undefined);
        std::vector<ResourceState> depthStates(mDepthAllocations.size(), ResourceState::Undefined);

        FrameGraphResources resources;
        resources.mRenderTextures = &mRenderInstances;
        resources.mDepthTextures = &mDepthInstances;
        resources.mRenderResourceInfos = &mRenderResourceInfos;
        resources.mDepthResourceInfos = &mDepthResourceInfos;

        for (int passIndex : mSortedPassIndices)
        {
            auto& node = mPassNodes[passIndex];

            std::vector<CD3DX12_RESOURCE_BARRIER> barriers;

            // Read barriers only on transition
            for (auto& h : node.ReadsRender)
            {
                auto& info = mRenderResourceInfos[h.Id];
                int idx = info.PhysAllocationIndex;
                if (renderStates[idx] != ResourceState::Read)
                {
                    RenderTexture& tex = resources.GetRenderTexture(h);
                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(tex.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
                    renderStates[idx] = ResourceState::Read;
                }
            }
            for (auto& h : node.ReadsDepth)
            {
                auto& info = mDepthResourceInfos[h.Id];
                int idx = info.PhysAllocationIndex;
                if (depthStates[idx] != ResourceState::Read)
                {
                    DepthTexture& tex = resources.GetDepthTexture(h);
                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(tex.resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ));
                    depthStates[idx] = ResourceState::Read;
                }
            }
            // Write barriers only on transition
            for (auto& h : node.WritesRender)
            {
                auto& info = mRenderResourceInfos[h.Id];
                int idx = info.PhysAllocationIndex;
                if (renderStates[idx] != ResourceState::Write)
                {
                    RenderTexture& tex = resources.GetRenderTexture(h);
                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(tex.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
                    renderStates[idx] = ResourceState::Write;
                }
            }
            for (auto& h : node.WritesDepth)
            {
                auto& info = mDepthResourceInfos[h.Id];
                int idx = info.PhysAllocationIndex;
                if (depthStates[idx] != ResourceState::Write)
                {
                    DepthTexture& tex = resources.GetDepthTexture(h);
                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(tex.resource, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
                    depthStates[idx] = ResourceState::Write;
                }
            }
            if (barriers.size())
                ctx.cmd->ResourceBarrier(static_cast<u32>(barriers.size()), barriers.data());
            node.Pass->Execute(ctx, resources);
        }
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