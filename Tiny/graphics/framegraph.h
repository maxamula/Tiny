// FrameGraph.h
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include "content/content.h"
#include "graphics/renderctx.h"

#include "taskflow/taskflow.hpp"


namespace tiny
{
	class RenderPassBase;

    constexpr uint32_t InvalidResourceId = UINT32_MAX;

    struct RenderTextureHandle
    {
        uint32_t Id;

        RenderTextureHandle()
            : Id(InvalidResourceId)
        {}

        explicit RenderTextureHandle(uint32_t id)
            : Id(id)
        {}
    };

    struct DepthTextureHandle
    {
        uint32_t Id;

        DepthTextureHandle()
            : Id(InvalidResourceId)
        {}

        explicit DepthTextureHandle(uint32_t id)
            : Id(id)
        {}
    };

    class FrameGraphResources;

    class TINYFX_API FrameGraph
    {
    public:
        class TINYFX_API Builder
        {
        public:
            Builder(FrameGraph& frameGraph, int passIndex);
            void Read(const RenderTextureHandle& handle);
            void Read(const DepthTextureHandle& handle);
            void Write(const RenderTextureHandle& handle);
            void Write(const DepthTextureHandle& handle);
        private:
            FrameGraph& mFrameGraph;
            int mPassIndex;
            template<typename HandleType>
            bool IsHandleInList(const HandleType& handle, const std::vector<HandleType>& list);
            friend class FrameGraph;
        };
    public:
        enum class ResourceState { Undefined, Read, Write };

        RenderTextureHandle CreateRenderTexture(const RenderTexture::Desc& desc);
        DepthTextureHandle CreateDepthTexture(const DepthTexture::Desc& desc);
        void Reset();
        FrameGraph();
        ~FrameGraph();

        template<typename PassType, typename... Args>
        PassType& AddPass(Args&&... args)
        {
            auto passPtr = std::make_shared<PassType>(std::forward<Args>(args)...);
            mPassNodes.emplace_back();
            int passIndex = static_cast<int>(mPassNodes.size()) - 1;
            mPassNodes[passIndex].Pass = passPtr;

            if (mDependencies.size() <= static_cast<size_t>(passIndex))
            {
                mDependencies.resize(mPassNodes.size());
            }

            Builder builder(*this, passIndex);
            passPtr->Setup(builder);

            return *passPtr;
        }

        void Compile(D3DContext& d3d);
        void ExecuteAsync(tf::Subflow& sf, D3DContext& d3d, RenderView& view, ID3D12Resource* pBackBuf);
    private:
        struct FrameTimingData
        {
            CComPtr<ID3D12QueryHeap>									timestampHeap;
            CComPtr<ID3D12Resource>										timestampResult;
            f64															lastGpuTime;
        };

        FrameTimingData mFrameTimingData;

        void AddDependency(int src, int dst);
        struct RenderResourceInfo
        {
            RenderTexture::Desc Desc;
            int ProducerPassIndex;
            int LastWriterPassIndex;
            std::vector<int> ReaderPassIndices;
            int FirstUseIndex;
            int LastUseIndex;
            int PhysAllocationIndex;
            D3D12_RESOURCE_STATES      InitialState;
        };

        struct DepthResourceInfo
        {
            DepthTexture::Desc Desc;
            int ProducerPassIndex;
            int LastWriterPassIndex;
            std::vector<int> ReaderPassIndices;
            int FirstUseIndex;
            int LastUseIndex;
            int PhysAllocationIndex;
            D3D12_RESOURCE_STATES      InitialState;
        };

        struct PassNode
        {
            std::shared_ptr<RenderPassBase> Pass;
            std::vector<RenderTextureHandle> ReadsRender;
            std::vector<DepthTextureHandle> ReadsDepth;
            std::vector<RenderTextureHandle> WritesRender;
            std::vector<DepthTextureHandle> WritesDepth;
        };

        struct Allocation
        {
            size_t Size;
            bool IsFree;
            bool IsInitialized;
        };

        std::vector<PassNode> mPassNodes;
        std::vector<RenderResourceInfo> mRenderResourceInfos;
        std::vector<DepthResourceInfo> mDepthResourceInfos;
        std::vector<std::vector<int>> mDependencies;
        std::vector<int> mSortedPassIndices;
        std::vector<Allocation> mRenderAllocations;
        std::vector<Allocation> mDepthAllocations;

        std::vector<RenderTexture> mRenderInstances;
        std::vector<DepthTexture> mDepthInstances;

        std::vector<D3D12_RESOURCE_STATES> mRenderStates;
        std::vector<D3D12_RESOURCE_STATES> mDepthStates;
        std::vector<std::vector<CD3DX12_RESOURCE_BARRIER>> mFrameBarriers;

    public:
        friend class FrameGraphResources;
    };

    class TINYFX_API FrameGraphResources
    {
    public:
        RenderTexture& GetRenderTexture(const RenderTextureHandle& handle);
        DepthTexture& GetDepthTexture(const DepthTextureHandle& handle);

    private:
        std::vector<RenderTexture>* mRenderTextures = nullptr;
        std::vector<DepthTexture>* mDepthTextures = nullptr;
        std::vector<FrameGraph::RenderResourceInfo>* mRenderResourceInfos = nullptr;
        std::vector<FrameGraph::DepthResourceInfo>* mDepthResourceInfos = nullptr;

        friend class FrameGraph;
    };

    template<typename HandleType>
    bool FrameGraph::Builder::IsHandleInList(const HandleType& handle, const std::vector<HandleType>& list)
    {
        return std::find_if(list.begin(), list.end(), [&](const HandleType& h) { return h.Id == handle.Id; }) != list.end();
    }
}