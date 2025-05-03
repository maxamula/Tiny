#pragma once
#include <d3d12.h>
#include <atlbase.h>
#include <tuple>
#include "graphics/descriptors.h"
#include "graphics/copyqueue.h"
#include "entt.hpp"
#include "graphics/renderctx.h"

using namespace entt;

namespace tiny
{
	enum InputAttributes
	{
		InputAttribute_Position = 0,
		InputAttribute_Normal = 1,
		InputAttribute_Tangent = 2,
		InputAttribute_UV = 3,
		InputAttribute_Color = 4,
		InputAttribute_BoneWeights = 5,
		InputAttributes_BoneIndices = 6,
		NumInputAttributes = 7
	};

	template<typename T>
	concept Resource = requires(T t)
	{
		{ T::Create() } -> std::same_as<T>;
		typename T::Desc;
	};

	struct TINYFX_API ICBuffer
	{
	public:
		virtual ~ICBuffer() = default;
		virtual std::tuple<void*, u32> GetData() = 0;
	};

	struct TINYFX_API ICBufferCPU : public ICBuffer
	{
	public:
		virtual ~ICBufferCPU() = default;
		void Update();
	_internal:
		CComPtr<ID3D12Resource> resource;
	};

	struct TINYFX_API ICBufferGPU : public ICBuffer
	{
	public:
		virtual ~ICBufferGPU() = default;
		void Update();
	_internal:
		CComPtr<ID3D12Resource> resource;
	};

	template<typename T>
	struct TINYFX_API CBufferCPU final : public ICBufferCPU
	{
	public:
		std::tuple<void*, u32> GetData() override { return { &data, sizeof(T) / sizeof(u32) }; }
		T data;
	private:
		static bool _sRegisterType()
		{
			entt::meta_factory<CBufferCPU<T>>()
				.type(entt::hashed_string{ ("CBufferCPU<" + std::string(typeid(T).name()) + ">").c_str() })
				.base<ICBufferCPU>()
				.data<&CBufferCPU<T>::data>("data"_hs);
			return true;
		}
		static inline const bool _sReg = _sRegisterType();
	};

	template<typename T>
	struct TINYFX_API CBufferGPU final : public ICBufferGPU
	{
	public:
		std::tuple<void*, u32> GetData() override { return { &data, sizeof(T) / sizeof(u32) }; }
		T data;
	private:
		static bool _sRegisterType()
		{
			entt::meta_factory<CBufferGPU<T>>()
				.type(entt::hashed_string{ ("CBufferGPU<" + std::string(typeid(T).name()) + ">").c_str() })
				.base<ICBufferCPU>()
				.data<&CBufferGPU<T>::data>("data"_hs);
			return true;
		}
		static inline const bool _sReg = _sRegisterType();
	};

	template<typename T>
	struct TINYFX_API CBufferInline final : public ICBuffer
	{
	public:
		T data;
		std::tuple<void*, u32> GetData() override { return { &data, sizeof(T) / sizeof(u32) }; }
	private:
		static bool _sRegisterType()
		{
			entt::meta_factory<CBufferInline<T>>()
				.type(entt::hashed_string{ ("CBufferInline<" + std::string(typeid(T).name()) + ">").c_str() })
				.base<ICBuffer>()
				.data<&CBufferInline<T>::data>("data"_hs);
			return true;
		}
		static inline const bool _sReg = _sRegisterType();
	};

	struct TINYFX_API Texture
	{
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle srv;
	};

	struct TINYFX_API Texture2D
	{
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle srv;
	};

	struct TINYFX_API RenderTexture
	{
		struct Desc
		{
			u32 width;
			u32 height;
			DXGI_FORMAT format;
			D3D12_CLEAR_VALUE clearValue;
		};
		static RenderTexture Create(const Desc& desc);
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle rtv;
		DescriptorHandle srv;
	};

	struct TINYFX_API DepthTexture
	{
		struct Desc
		{
			u32 width;
			u32 height;
		};
		static DepthTexture Create(const Desc& desc);
	_internal:
		CComPtr<ID3D12Resource> resource;
		DescriptorHandle dsv;
		DescriptorHandle srv;
	};

	struct TINYFX_API Mesh
	{
	_internal:
		CComPtr<ID3D12Resource> vertexBuffer;
		CComPtr<ID3D12Resource> indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		u32 numIndices;
		u32 numVertices;
		u8 inputAttributes;
	};

	std::vector<D3D12_INPUT_ELEMENT_DESC> ConstructInputLayout(u8 attributes);
	u32 GetVertexStride(u8 attributes);
}