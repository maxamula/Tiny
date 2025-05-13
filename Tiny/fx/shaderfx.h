#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <atlbase.h>
#include <unordered_map>
#include <stdexcept>
#include <DirectXMath.h>

#include "../entt.hpp"
#include "d3dx12.hpp"
#include <variant>

#define TINY_DECLARE_MAT_ID(name) u64 ID() override { return #name##_hs; }

namespace tiny::fx
{
#pragma region Common
	enum ShaderSpecialBindType
	{
		ShaderSpecialBind_World,
		ShaderSpecialBind_Light,
		ShaderSpecialBind_Environment,
	};

	enum ShaderCBufferType
	{
		ShaderCBufferType_Constants,
		ShaderCBufferType_CPU,
		ShaderCBufferType_GPU,
	};

	struct ShaderResourceSpecialBinding
	{
		u32 rootParamIndex;
		ShaderSpecialBindType special;
	};

	struct ShaderResourceBinding
	{
		D3D_SHADER_INPUT_TYPE type;
		u64 fieldID;
		u32 rootParamIndex;
		union
		{
			ShaderCBufferType cbufferType;
		} details;
	};

#pragma endregion

#pragma region Default Material 
	struct ShaderFX
	{
		CComPtr<IDxcBlob>										vs;
		CComPtr<IDxcBlob>										ps;
		CComPtr<ID3D12RootSignature>							rootSig;
		CComPtr<ID3DBlob>										rootSigBlob;
		CComPtr<ID3D12PipelineState>							pso;
		std::vector<ShaderResourceBinding>						bindings;
		std::vector<ShaderResourceSpecialBinding>				specialBindings;
	};

	struct TINYFX_API IMaterialInstance
	{
		virtual ~IMaterialInstance() = default;
		virtual u64 ID() = 0;
		virtual entt::meta_any Meta() = 0;
		void SetFeatureFlags(u8 flags);
	_internal:
		ShaderFX* fx;
	};

	// CRTP
	template<typename Derived>
	struct MaterialInstance : public IMaterialInstance
	{
		virtual ~MaterialInstance() = default;
		entt::meta_any Meta() override { return entt::forward_as_meta(static_cast<Derived&>(*this));}
	private:
		static bool _sRegisterType()
		{
			entt::meta_factory<MaterialInstance<Derived>>()
				.type(entt::hashed_string{ ("MaterialInstance<" + std::string(typeid(Derived).name()) + ">").c_str() })
				.base<IMaterialInstance>();
			return true;
		}
		static inline const bool _sReg = _sRegisterType();
	};

	struct Material final
	{
		virtual ~Material() = default;
		std::unordered_map<u64, ShaderFX> fx;
	};
#pragma endregion

#pragma region Mesh Material
	struct MeshShaderFX
	{
		CComPtr<IDxcBlob>										vs;
		CComPtr<IDxcBlob>										ps;
		CComPtr<ID3D12RootSignature>							rootSig;
		CComPtr<ID3DBlob>										rootSigBlob;
		std::unordered_map<u64, CComPtr<ID3D12PipelineState>>	pso;
		std::vector<ShaderResourceBinding>						bindings;
		std::vector<ShaderResourceSpecialBinding>				specialBindings;
	};

	struct TINYFX_API IMeshMaterialInstance
	{
		virtual ~IMeshMaterialInstance() = default;
		virtual u64 ID() = 0;
		virtual entt::meta_any Meta() = 0;
		void SetFeatureFlags(u8 flags);
	_internal:
		MeshShaderFX* fx;
	};

	template<typename Derived>
	struct TINYFX_API MeshMaterialInstance : public IMeshMaterialInstance
	{
		virtual ~MeshMaterialInstance() = default;
		entt::meta_any Meta() override { return entt::forward_as_meta(static_cast<Derived&>(*this)); }
	private:
		static bool _sRegisterType()
		{
			entt::meta_factory<MeshMaterialInstance<Derived>>()
				.type(entt::hashed_string{ ("MeshMaterialInstance<" + std::string(typeid(Derived).name()) + ">").c_str() })
				.base<IMeshMaterialInstance>();
			return true;
		}
		static inline const bool _sReg = _sRegisterType();
	};

	struct MeshMaterial final
	{
		virtual ~MeshMaterial() = default;
		std::unordered_map<u64, MeshShaderFX> fx;
	};

	struct MeshTechnique
	{
		enum RenderPassIds : u16
		{
			RenderPassId_Opaque = 0,
			RenderPassId_Transparent = 1,
			RenderPassId_Deferred = 2,
			RenderPassId_DeferredLight = 3,
			RenderPassId_Shadow = 4,
		};
		std::unordered_map<u16, std::shared_ptr<IMeshMaterialInstance>> passes;
	};
#pragma endregion

#pragma region Material Creation Tools
	enum MacroRules : unsigned char
	{
		MacroRule_None = 0,
		MacroRule_Conflict = 1,
		MacroRule_Dependency = 2
	};

	struct TINYFX_API FxCompilationDesc final
	{
		LPCSTR name;
		struct
		{
			LPCSTR sourcePath;
			LPCSTR entryPoint;
		} vs;
		struct
		{
			LPCSTR sourcePath;
			LPCSTR entryPoint;
		} ps;
	};

	struct TINYFX_API FxVariantsCompilationDesc final
	{
		struct
		{
			LPCSTR sourcePath;
			LPCSTR entryPoint;
		} vs;
		struct
		{
			LPCSTR sourcePath;
			LPCSTR entryPoint;
		} ps;

		LPCSTR defines[MAX_CONDITIONAL_COMPILATIONS];
		unsigned char numConditionalCompilations;
		MacroRules rules[MAX_CONDITIONAL_COMPILATIONS][MAX_CONDITIONAL_COMPILATIONS];
	};

	struct IntermediateCompiledCombination
	{
		u8 combinationId;
		CComPtr<IDxcBlob> vs;
		CComPtr<IDxcBlob> ps;
	};

	struct IntermediateShaderResource
	{
		char						name[TINY_MAX_SHADER_RESOURCE_NAME];
		D3D_SHADER_INPUT_TYPE       type;
		u32	                        bindPoint;
		u32		                    bindCount;
		u32	                        cbufferSize;
	};

	struct IntermediateCompiledReflection
	{
		std::vector<IntermediateShaderResource> resources;
		u8 mandatoryInputAttributes;
	};

	TINYFX_API void CompileFx(FxCompilationDesc& desc, IntermediateCompiledCombination& oCompiled);
	TINYFX_API void CompileFxVariants(const FxVariantsCompilationDesc& desc, std::vector<IntermediateCompiledCombination>& oCompiled);
	TINYFX_API void GetReflectionInfo(const IntermediateCompiledCombination& combination, IntermediateCompiledReflection& oReflection);
	TINYFX_API void InitializeRootParameter(const IntermediateShaderResource& resource, CD3DX12_ROOT_PARAMETER& oRootParameter);
	TINYFX_API bool ProcessSpecialBindings(const IntermediateShaderResource& resource, u32 rootIndex, u64 propertyID, std::vector<ShaderResourceSpecialBinding>& bindings);
	TINYFX_API void GenInputVariantsPSOs(D3D12_GRAPHICS_PIPELINE_STATE_DESC& baseDesc, ID3D12Device* pDevice, u8 mandatoryAttributes, std::unordered_map<u64, CComPtr<ID3D12PipelineState>>& oPso);

	struct StaticSamplerDesc
	{
		u64 id;
		D3D12_STATIC_SAMPLER_DESC desc;
	};

	struct MaterialInitializationDesc
	{
		FxVariantsCompilationDesc fxDesc;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;
		std::vector<StaticSamplerDesc> staticSamplers;
	};

	TINYFX_API void MaterialDefaultInitialize(ID3D12Device* pDevice, const MaterialInitializationDesc& desc, Material& material);
	TINYFX_API void MeshMaterialDefaultInitialize(ID3D12Device* pDevice, const MaterialInitializationDesc& desc, MeshMaterial& material);

#pragma endregion
}