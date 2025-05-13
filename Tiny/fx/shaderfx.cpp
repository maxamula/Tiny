#include "shaderfx.h"
#include <d3d12shader.h>
#include <dxcapi.h>
#include <fstream>
#include <string.h>

#include "registry_internal.h"
#include "content/content.h"
#include "meta.h"

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d12.lib")


namespace tiny::fx
{
	META(IMaterialInstance){ meta.data<&IMaterialInstance::fx>("fx"_hs); }
	META(IMeshMaterialInstance){ meta.data<&IMeshMaterialInstance::fx>("fx"_hs); }

	namespace
	{
		CComPtr<IDxcCompiler3> gDxcCompiler;
		CComPtr<IDxcIncludeHandler> gDxcIncludeHandler;
		CComPtr<IDxcUtils> gDxcUtils;

		bool ConvertToLPWSTR(const char* c_str, LPWSTR buffer, int bufferLength)
		{
			if (!c_str || !buffer || bufferLength <= 0)
				return false;

			int requiredSize = MultiByteToWideChar(CP_UTF8, 0, c_str, -1, nullptr, 0);
			if (requiredSize == 0 || requiredSize > bufferLength)
				return false;

			int result = MultiByteToWideChar(CP_UTF8, 0, c_str, -1, buffer, bufferLength);
			return (result > 0);
		}

		[[nodiscard]] bool CheckCombinationCompatibility(const FxVariantsCompilationDesc& desc, unsigned char combination)
		{
			for (unsigned char i = 0; i < desc.numConditionalCompilations; i++)
			{
				for (unsigned char j = i + 1; j < desc.numConditionalCompilations; j++)
				{
					if (desc.rules[i][j] == MacroRule_Conflict)
					{
						if ((combination & (1 << i)) && (combination & (1 << j)))
						{
							return false;
						}
					}
					else if (desc.rules[i][j] == MacroRule_Dependency)
					{
						if ((combination & (1 << i)) && !(combination & (1 << j)))
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		enum ShaderRegister : u8
		{
			ShaderRegister_B,
			ShaderRegister_T,
			ShaderRegister_S,
			ShaderRegister_U
		};

		ShaderRegister D3DShaderInputTypeToRegisterType(D3D_SHADER_INPUT_TYPE inputType)
		{
			switch (inputType)
			{
			case D3D_SIT_CBUFFER:
				return ShaderRegister_B;
			case D3D_SIT_TBUFFER:
				return ShaderRegister_T;
			case D3D_SIT_TEXTURE:
				return ShaderRegister_T;
			case D3D_SIT_SAMPLER:
				return ShaderRegister_S;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				return ShaderRegister_U;
			case D3D_SIT_STRUCTURED:
				return ShaderRegister_T;
			case D3D_SIT_BYTEADDRESS:
				return ShaderRegister_T;
			case D3D_SIT_RTACCELERATIONSTRUCTURE:
				return ShaderRegister_T;
			default:
				THROW_ENGINE_EXCEPTION("Unknown shader input type");
			}
		}
	} // namespace anonymous

	void IMaterialInstance::SetFeatureFlags(u8 flags)
	{
		auto& matReg = fx::GetMaterialsRegistry();
		Material& material = matReg[ID()].material;
		auto it = material.fx.find(flags);
		if (it != material.fx.end())
			fx = &it->second;
		else
			THROW_ENGINE_EXCEPTION("Invalid feature flags: {}", flags);
	}

	void IMeshMaterialInstance::SetFeatureFlags(u8 flags)
	{
		auto& matReg = fx::GetMeshMaterialsRegistry();
		MeshMaterial& material = matReg[ID()].material;
		auto it = material.fx.find(flags);
		if (it != material.fx.end())
			fx = &it->second;
		else
			THROW_ENGINE_EXCEPTION("Invalid feature flags: {}", flags);
	}

#pragma region Material Creation Tools

	void CompileFx(FxCompilationDesc& desc, IntermediateCompiledCombination& oCompiled)
	{
		std::vector<LPCWSTR> commonArgs
		{
			DXC_ARG_ALL_RESOURCES_BOUND,
	#if _DEBUG
			DXC_ARG_DEBUG,
			DXC_ARG_SKIP_OPTIMIZATIONS,
	#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
	#endif
			L"Qstrip_reflect",
			L"Qstrip_debug",
		};

		{
			std::string path(desc.vs.sourcePath);
			std::vector<char> vsData;

			if (path.compare(0, 4, "res:") == 0)
			{
				std::string rem = path.substr(4);
				size_t pos = rem.find("::");
				if (pos != std::string::npos)
				{
					std::string dllName = rem.substr(0, pos);
					std::string resourceName = rem.substr(pos + 2);
					HMODULE hModule = GetModuleHandleA(dllName.c_str());
					if (!hModule)
						THROW_ENGINE_EXCEPTION("Failed to get {}.dll module handle", dllName);
					HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), "SHADERS");
					if (!hResource)
						THROW_ENGINE_EXCEPTION("Failed to find resource {} in {}.dll", resourceName, dllName);
					DWORD size = SizeofResource(hModule, hResource);
					if (!size)
						THROW_ENGINE_EXCEPTION("Invalid resource size for {} in {}.dll", resourceName, dllName);
					HGLOBAL hGlobal = LoadResource(hModule, hResource);
					if (!hGlobal)
						THROW_ENGINE_EXCEPTION("Failed to load resource {} in {}.dll", resourceName, dllName);

					LPVOID pData = LockResource(hGlobal);
					if (!pData)
						THROW_ENGINE_EXCEPTION("Failed to lock resource {} in {}.dll", resourceName, dllName);

					vsData.resize(size);
					memcpy(vsData.data(), pData, size);

					UnlockResource(hGlobal);
					FreeResource(hGlobal);
				}
			}
			else
			{
				std::fstream vsFile(desc.vs.sourcePath, std::ios::in | std::ios::binary);
				if (vsFile.is_open())
				{
					vsData = std::vector<char>((std::istreambuf_iterator<char>(vsFile)), std::istreambuf_iterator<char>());
				}
				else
					THROW_ENGINE_EXCEPTION("Failed to open vertex shader file");
			}

			WCHAR entryPoint[TINY_MAX_ENTRYPOINT_NAME];
			if (!ConvertToLPWSTR(desc.vs.entryPoint, entryPoint, TINY_MAX_ENTRYPOINT_NAME))
				THROW_ENGINE_EXCEPTION("Failed to convert entry point to LPWSTR");
			std::vector<LPCWSTR> args
			{
				L"-T", L"vs_6_0",
				L"-E", entryPoint
			};

			args.insert(args.end(), commonArgs.begin(), commonArgs.end());
			CComPtr<IDxcBlobEncoding> pSourceBlob;
			THROW_IF_FAILED(gDxcUtils->CreateBlobFromPinned(vsData.data(), vsData.size(), CP_UTF8, &pSourceBlob));
			DxcBuffer buffer
			{
				.Ptr = pSourceBlob->GetBufferPointer(),
				.Size = pSourceBlob->GetBufferSize(),
				.Encoding = CP_UTF8
			};

			CComPtr<IDxcResult> compileResult;
			THROW_IF_FAILED(gDxcCompiler->Compile(&buffer, args.data(), static_cast<int>(args.size()), gDxcIncludeHandler.p, IID_PPV_ARGS(&compileResult)));

			HRESULT hr;
			THROW_IF_FAILED(compileResult->GetStatus(&hr));
			if (FAILED(hr))
			{
				CComPtr<IDxcBlobEncoding> pErrorBlob;
				THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBlob), nullptr));
				std::string error((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
				THROW_ENGINE_EXCEPTION("Failed to compile vertex shader: {}", error);
			}

			CComPtr<IDxcBlob> pBytecodeBlob;
			THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pBytecodeBlob), nullptr));
			oCompiled.vs = pBytecodeBlob;
		}

		{
			std::string path(desc.ps.sourcePath);
			std::vector<char> psData;

			if (path.compare(0, 4, "res:") == 0)
			{
				std::string rem = path.substr(4);
				size_t pos = rem.find("::");
				if (pos != std::string::npos)
				{
					std::string dllName = rem.substr(0, pos);
					std::string resourceName = rem.substr(pos + 2);
					HMODULE hModule = GetModuleHandleA(dllName.c_str());
					if (!hModule)
						THROW_ENGINE_EXCEPTION("Failed to get {}.dll module handle", dllName);
					HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), "SHADERS");
					if (!hResource)
						THROW_ENGINE_EXCEPTION("Failed to find resource {} in {}.dll", resourceName, dllName);
					DWORD size = SizeofResource(hModule, hResource);
					if (!size)
						THROW_ENGINE_EXCEPTION("Invalid resource size for {} in {}.dll", resourceName, dllName);
					HGLOBAL hGlobal = LoadResource(hModule, hResource);
					if (!hGlobal)
						THROW_ENGINE_EXCEPTION("Failed to load resource {} in {}.dll", resourceName, dllName);

					LPVOID pData = LockResource(hGlobal);
					if (!pData)
						THROW_ENGINE_EXCEPTION("Failed to lock resource {} in {}.dll", resourceName, dllName);

					psData.resize(size);
					memcpy(psData.data(), pData, size);

					UnlockResource(hGlobal);
					FreeResource(hGlobal);
				}
			}
			else
			{
				std::fstream psFile(desc.ps.sourcePath, std::ios::in | std::ios::binary);
				if (psFile.is_open())
				{
					psData = std::vector<char>((std::istreambuf_iterator<char>(psFile)), std::istreambuf_iterator<char>());
				}
				else
					THROW_ENGINE_EXCEPTION("Failed to open pixel shader file");
			}

			WCHAR entryPoint[TINY_MAX_ENTRYPOINT_NAME];
			if (!ConvertToLPWSTR(desc.ps.entryPoint, entryPoint, TINY_MAX_ENTRYPOINT_NAME))
				THROW_ENGINE_EXCEPTION("Failed to convert entry point to LPWSTR");
			std::vector<LPCWSTR> args
			{
				L"-T", L"ps_6_0",
				L"-E", entryPoint
			};

			args.insert(args.end(), commonArgs.begin(), commonArgs.end());
			CComPtr<IDxcBlobEncoding> pSourceBlob;
			THROW_IF_FAILED(gDxcUtils->CreateBlobFromPinned(psData.data(), psData.size(), CP_UTF8, &pSourceBlob));
			DxcBuffer buffer
			{
				.Ptr = pSourceBlob->GetBufferPointer(),
				.Size = pSourceBlob->GetBufferSize(),
				.Encoding = CP_UTF8
			};

			CComPtr<IDxcResult> compileResult;
			THROW_IF_FAILED(gDxcCompiler->Compile(&buffer, args.data(), static_cast<int>(args.size()), gDxcIncludeHandler.p, IID_PPV_ARGS(&compileResult)));

			HRESULT hr;
			THROW_IF_FAILED(compileResult->GetStatus(&hr));
			if (FAILED(hr))
			{
				CComPtr<IDxcBlobEncoding> pErrorBlob;
				THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBlob), nullptr));
				std::string error((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
				THROW_ENGINE_EXCEPTION("Failed to compile pixel shader: {}", error);
			}

			CComPtr<IDxcBlob> pBytecodeBlob;
			THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pBytecodeBlob), nullptr));

			oCompiled.ps = pBytecodeBlob;
		}
	}

	void CompileFxVariants(const FxVariantsCompilationDesc& desc, std::vector<IntermediateCompiledCombination>& oCompiled)
	{
		std::unordered_map<u64, CComPtr<IDxcBlob>> vsCache;
		std::unordered_map<u64, CComPtr<IDxcBlob>> psCache;
		std::vector<LPCWSTR> commonArgs
		{
			DXC_ARG_ALL_RESOURCES_BOUND,
	#if _DEBUG
			DXC_ARG_DEBUG,
			DXC_ARG_SKIP_OPTIMIZATIONS,
	#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
	#endif
			L"Qstrip_reflect",
			L"Qstrip_debug",
		};

		auto fnHashShaderBlob = [](const void* data, size_t size) -> u64
		{
			return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(data), size));
		};

		unsigned char combinations = 1 << desc.numConditionalCompilations;

		for (unsigned char combination = 0; combination < combinations; combination++)
		{
			if (CheckCombinationCompatibility(desc, combination))
			{
				IntermediateCompiledCombination compiledCombination;

				{
					std::string path(desc.vs.sourcePath);
					std::vector<char> vsData;

					if (path.compare(0, 4, "res:") == 0)
					{
						std::string rem = path.substr(4);
						size_t pos = rem.find("::");
						if (pos != std::string::npos)
						{
							std::string dllName = rem.substr(0, pos);
							std::string resourceName = rem.substr(pos + 2);
							HMODULE hModule = nullptr;
							if (dllName == "MainModule")
								hModule = GetModuleHandleA(nullptr);
							else
								hModule = GetModuleHandleA(dllName.c_str());
							if (!hModule)
								THROW_ENGINE_EXCEPTION("Failed to get {}.dll module handle", dllName);
							HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), "SHADERS");
							if (!hResource)
								THROW_ENGINE_EXCEPTION("Failed to find resource {} in {}.dll", resourceName, dllName);
							DWORD size = SizeofResource(hModule, hResource);
							if (!size)
								THROW_ENGINE_EXCEPTION("Invalid resource size for {} in {}.dll", resourceName, dllName);
							HGLOBAL hGlobal = LoadResource(hModule, hResource);
							if (!hGlobal)
								THROW_ENGINE_EXCEPTION("Failed to load resource {} in {}.dll", resourceName, dllName);

							LPVOID pData = LockResource(hGlobal);
							if (!pData)
								THROW_ENGINE_EXCEPTION("Failed to lock resource {} in {}.dll", resourceName, dllName);

							vsData.resize(size);
							memcpy(vsData.data(), pData, size);

							UnlockResource(hGlobal);
							FreeResource(hGlobal);
						}
					}
					else
					{
						std::fstream vsFile(desc.vs.sourcePath, std::ios::in | std::ios::binary);
						if (vsFile.is_open())
						{
							vsData = std::vector<char>((std::istreambuf_iterator<char>(vsFile)), std::istreambuf_iterator<char>());
						}
						else
							THROW_ENGINE_EXCEPTION("Failed to open vertex shader file");
					}

					WCHAR entryPoint[TINY_MAX_ENTRYPOINT_NAME];
					if (!ConvertToLPWSTR(desc.vs.entryPoint, entryPoint, TINY_MAX_ENTRYPOINT_NAME))
						THROW_ENGINE_EXCEPTION("Failed to convert entry point to LPWSTR");
					std::vector<LPCWSTR> args
					{
						L"-T", L"vs_6_0",
						L"-E", entryPoint
					};
					for (unsigned char i = 0; i < sizeof(unsigned char); i++)
					{
						if (combination & (1u << i))
						{
							args.push_back(L"-D");
							WCHAR macro[64];
							if (!ConvertToLPWSTR(desc.defines[i], macro, 64))
								THROW_ENGINE_EXCEPTION("Failed to convert macro to LPWSTR");
							args.push_back(macro);
						}
					}

					args.insert(args.end(), commonArgs.begin(), commonArgs.end());
					CComPtr<IDxcBlobEncoding> pSourceBlob;
					THROW_IF_FAILED(gDxcUtils->CreateBlobFromPinned(vsData.data(), vsData.size(), CP_UTF8, &pSourceBlob));
					DxcBuffer buffer
					{
						.Ptr = pSourceBlob->GetBufferPointer(),
						.Size = pSourceBlob->GetBufferSize(),
						.Encoding = CP_UTF8
					};

					CComPtr<IDxcResult> compileResult;
					THROW_IF_FAILED(gDxcCompiler->Compile(&buffer, args.data(), static_cast<int>(args.size()), gDxcIncludeHandler.p, IID_PPV_ARGS(&compileResult)));

					HRESULT hr;
					THROW_IF_FAILED(compileResult->GetStatus(&hr));
					if (FAILED(hr))
					{
						CComPtr<IDxcBlobEncoding> pErrorBlob;
						THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBlob), nullptr));
						std::string error((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
						THROW_ENGINE_EXCEPTION("Failed to compile vertex shader: {}", error);
					}

					CComPtr<IDxcBlob> pBytecodeBlob;
					THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pBytecodeBlob), nullptr));

					u64 hash = fnHashShaderBlob(pBytecodeBlob->GetBufferPointer(), pBytecodeBlob->GetBufferSize());
					auto it = vsCache.find(hash);
					if (it == vsCache.end())
					{
						vsCache[hash] = pBytecodeBlob;
						compiledCombination.vs = pBytecodeBlob;
					}
					else
						compiledCombination.vs = it->second;
				}

				{
					std::string path(desc.ps.sourcePath);
					std::vector<char> psData;

					if (path.compare(0, 4, "res:") == 0)
					{
						std::string rem = path.substr(4);
						size_t pos = rem.find("::");
						if (pos != std::string::npos)
						{
							std::string dllName = rem.substr(0, pos);
							std::string resourceName = rem.substr(pos + 2);
							HMODULE hModule = nullptr;
							if (dllName == "MainModule")
								hModule = GetModuleHandleA(nullptr);
							else
								hModule	= GetModuleHandleA(dllName.c_str());
							if (!hModule)
								THROW_ENGINE_EXCEPTION("Failed to get {}.dll module handle", dllName);
							HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), "SHADERS");
							if (!hResource)
								THROW_ENGINE_EXCEPTION("Failed to find resource {} in {}.dll", resourceName, dllName);
							DWORD size = SizeofResource(hModule, hResource);
							if (!size)
								THROW_ENGINE_EXCEPTION("Invalid resource size for {} in {}.dll", resourceName, dllName);
							HGLOBAL hGlobal = LoadResource(hModule, hResource);
							if (!hGlobal)
								THROW_ENGINE_EXCEPTION("Failed to load resource {} in {}.dll", resourceName, dllName);

							LPVOID pData = LockResource(hGlobal);
							if (!pData)
								THROW_ENGINE_EXCEPTION("Failed to lock resource {} in {}.dll", resourceName, dllName);

							psData.resize(size);
							memcpy(psData.data(), pData, size);

							UnlockResource(hGlobal);
							FreeResource(hGlobal);
						}
					}
					else
					{
						std::fstream psFile(desc.ps.sourcePath, std::ios::in | std::ios::binary);
						if (psFile.is_open())
						{
							psData = std::vector<char>((std::istreambuf_iterator<char>(psFile)), std::istreambuf_iterator<char>());
						}
						else
							THROW_ENGINE_EXCEPTION("Failed to open pixel shader file");
					}

					WCHAR entryPoint[TINY_MAX_ENTRYPOINT_NAME];
					if (!ConvertToLPWSTR(desc.ps.entryPoint, entryPoint, TINY_MAX_ENTRYPOINT_NAME))
						THROW_ENGINE_EXCEPTION("Failed to convert entry point to LPWSTR");
					std::vector<LPCWSTR> args
					{
						L"-T", L"ps_6_0",
						L"-E", entryPoint
					};
					for (unsigned char i = 0; i < sizeof(unsigned char); i++)
					{
						if (combination & (1u << i))
						{
							args.push_back(L"-D");
							WCHAR macro[64];
							if (!ConvertToLPWSTR(desc.defines[i], macro, 64))
								THROW_ENGINE_EXCEPTION("Failed to convert macro to LPWSTR");
							args.push_back(macro);
						}
					}

					args.insert(args.end(), commonArgs.begin(), commonArgs.end());
					CComPtr<IDxcBlobEncoding> pSourceBlob;
					THROW_IF_FAILED(gDxcUtils->CreateBlobFromPinned(psData.data(), psData.size(), CP_UTF8, &pSourceBlob));
					DxcBuffer buffer
					{
						.Ptr = pSourceBlob->GetBufferPointer(),
						.Size = pSourceBlob->GetBufferSize(),
						.Encoding = CP_UTF8
					};

					CComPtr<IDxcResult> compileResult;
					THROW_IF_FAILED(gDxcCompiler->Compile(&buffer, args.data(), static_cast<int>(args.size()), gDxcIncludeHandler.p, IID_PPV_ARGS(&compileResult)));

					HRESULT hr;
					THROW_IF_FAILED(compileResult->GetStatus(&hr));
					if (FAILED(hr))
					{
						CComPtr<IDxcBlobEncoding> pErrorBlob;
						THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBlob), nullptr));
						std::string error((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
						THROW_ENGINE_EXCEPTION("Failed to compile pixel shader: {}", error);
					}

					CComPtr<IDxcBlob> pBytecodeBlob;
					THROW_IF_FAILED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pBytecodeBlob), nullptr));

					u64 hash = fnHashShaderBlob(pBytecodeBlob->GetBufferPointer(), pBytecodeBlob->GetBufferSize());
					auto it = psCache.find(hash);
					if (it == psCache.end())
					{
						psCache[hash] = pBytecodeBlob;
						compiledCombination.ps = pBytecodeBlob;
					}
					else
						compiledCombination.ps = it->second;
				}
				compiledCombination.combinationId = combination;
				oCompiled.push_back(compiledCombination);
			}
		}
	}

	void GetReflectionInfo(const IntermediateCompiledCombination& combination, IntermediateCompiledReflection& oReflection)
	{
		std::unordered_map<u16, IntermediateShaderResource> resourcesCache;

		auto fnHashBasedOnTypeAndShaderRegister = [](ShaderRegister type, UINT registerSlot) -> u16
		{
			return (static_cast<u16>(type) << 8) | registerSlot;
		};

		{
			CComPtr<ID3D12ShaderReflection> shaderReflection;
			DxcBuffer buffer
			{
				.Ptr = combination.vs->GetBufferPointer(),
				.Size = combination.vs->GetBufferSize(),
				.Encoding = 0
			};
			THROW_IF_FAILED(gDxcUtils->CreateReflection(&buffer, IID_PPV_ARGS(&shaderReflection)));

			D3D12_SHADER_DESC shaderDesc;
			shaderReflection->GetDesc(&shaderDesc);

			for (UINT32 i = 0; i < shaderDesc.BoundResources; i++)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				shaderReflection->GetResourceBindingDesc(i, &bindDesc);
				u16 hash = fnHashBasedOnTypeAndShaderRegister(D3DShaderInputTypeToRegisterType(bindDesc.Type), bindDesc.BindPoint);
				resourcesCache[hash] = IntermediateShaderResource
				{
					.type = bindDesc.Type,
					.bindPoint = bindDesc.BindPoint,
					.bindCount = bindDesc.BindCount
				};
				strcpy_s(resourcesCache[hash].name, bindDesc.Name);
			}

			oReflection.mandatoryInputAttributes = 0;
			for (UINT32 i = 0; i < shaderDesc.InputParameters; i++)
			{
				D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
				shaderReflection->GetInputParameterDesc(i, &paramDesc);
				if (strcmp(paramDesc.SemanticName, "NORMAL") == 0)
					oReflection.mandatoryInputAttributes |= InputAttribute_Normal;
				else if (strcmp(paramDesc.SemanticName, "TANGENT") == 0)
					oReflection.mandatoryInputAttributes |= InputAttribute_Tangent;
				else if (strcmp(paramDesc.SemanticName, "TEXCOORD") == 0)
					oReflection.mandatoryInputAttributes |= InputAttribute_UV;
				else if (strcmp(paramDesc.SemanticName, "COLOR") == 0)
					oReflection.mandatoryInputAttributes |= InputAttribute_Color;
				else if (strcmp(paramDesc.SemanticName, "BONE_WEIGHTS") == 0)
					oReflection.mandatoryInputAttributes |= InputAttribute_BoneWeights;
				else if (strcmp(paramDesc.SemanticName, "BONE_INDICES") == 0)
					oReflection.mandatoryInputAttributes |= InputAttributes_BoneIndices;
			}
		}

		{
			CComPtr<ID3D12ShaderReflection> shaderReflection;
			DxcBuffer buffer
			{
				.Ptr = combination.ps->GetBufferPointer(),
				.Size = combination.ps->GetBufferSize(),
				.Encoding = 0
			};
			HRESULT g = gDxcUtils->CreateReflection(&buffer, IID_PPV_ARGS(&shaderReflection));

			D3D12_SHADER_DESC shaderDesc;
			shaderReflection->GetDesc(&shaderDesc);

			for (UINT32 i = 0; i < shaderDesc.BoundResources; i++)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				shaderReflection->GetResourceBindingDesc(i, &bindDesc);
				u16 hash = fnHashBasedOnTypeAndShaderRegister(D3DShaderInputTypeToRegisterType(bindDesc.Type), bindDesc.BindPoint);
				resourcesCache[hash] = IntermediateShaderResource
				{
					.type = bindDesc.Type,
					.bindPoint = bindDesc.BindPoint,
					.bindCount = bindDesc.BindCount
				};
				strcpy_s(resourcesCache[hash].name, bindDesc.Name);
			}

			// Iterate through cbuffers and fill sizes in reflection
			for (UINT32 i = 0; i < shaderDesc.ConstantBuffers; i++)
			{
				ID3D12ShaderReflectionConstantBuffer* pCB = shaderReflection->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC cbDesc;
				pCB->GetDesc(&cbDesc);
				// find corresponding resource
				for (UINT32 j = 0; j < shaderDesc.BoundResources; j++)
				{
					D3D12_SHADER_INPUT_BIND_DESC bindDesc;
					shaderReflection->GetResourceBindingDesc(j, &bindDesc);
					if (strcmp(bindDesc.Name, cbDesc.Name) == 0)
					{
						u16 hash = fnHashBasedOnTypeAndShaderRegister(D3DShaderInputTypeToRegisterType(bindDesc.Type), bindDesc.BindPoint);
						resourcesCache[hash].cbufferSize = cbDesc.Size;
						break;
					}
				}
			}
		}

		for (auto& [key, value] : resourcesCache)
		{
			oReflection.resources.push_back(value);
		}
	}

	void InitializeRootParameter(const IntermediateShaderResource& resource, CD3DX12_ROOT_PARAMETER& oRootParameter)
	{
		const UINT spaceIndex = 0;

		switch (resource.type)
		{
		case D3D_SIT_CBUFFER:
			//oRootParameter.InitAsConstantBufferView(resource.bindPoint, spaceIndex, D3D12_SHADER_VISIBILITY_ALL);
			// init as constants
			oRootParameter.InitAsConstants(3, resource.bindPoint, spaceIndex, D3D12_SHADER_VISIBILITY_ALL);
			break;

		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
		{
			static CD3DX12_DESCRIPTOR_RANGE srvRange = {};
			srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, resource.bindCount, resource.bindPoint, spaceIndex);
			oRootParameter.InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
			std::cout << "Texture: " << resource.bindPoint << " " << resource.bindCount << std::endl;
			break;
		}

		case D3D_SIT_SAMPLER:
		{
			static CD3DX12_DESCRIPTOR_RANGE samplerRange;
			samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, resource.bindCount, resource.bindPoint, spaceIndex, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
			oRootParameter.InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_ALL);
			break;
		}

		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		{
			static CD3DX12_DESCRIPTOR_RANGE uavRange;
			uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, resource.bindCount, resource.bindPoint, spaceIndex, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
			oRootParameter.InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
			break;
		}

		default:
			throw std::runtime_error("Unsupported shader resource type in InitializeRootParameter");
		}
	}

	bool ProcessSpecialBindings(const IntermediateShaderResource& resource, u32 rootIndex, u64 propertyID, std::vector<ShaderResourceSpecialBinding>& bindings)
	{
		switch (propertyID)
		{
		case "cbWorld"_hs:
			if (resource.type == D3D_SIT_CBUFFER)
				bindings.push_back(ShaderResourceSpecialBinding{ rootIndex, ShaderSpecialBind_World });
			return true;
		case "cbLight"_hs:
			if (resource.type == D3D_SIT_CBUFFER)
				bindings.push_back(ShaderResourceSpecialBinding{ rootIndex, ShaderSpecialBind_Light });
			return true;
		case "cbEnvironment"_hs:
			if (resource.type == D3D_SIT_CBUFFER)
				bindings.push_back(ShaderResourceSpecialBinding{ rootIndex, ShaderSpecialBind_Environment });
			return true;
		default:
			return false;
		}
	}


	void GenInputVariantsPSOs(D3D12_GRAPHICS_PIPELINE_STATE_DESC& baseDesc, ID3D12Device* pDevice, u8 mandatoryAttributes, std::unordered_map<u64, CComPtr<ID3D12PipelineState>>& oPso)
	{
		for (u8 i = 0; i < (1 << NumInputAttributes); i++)
		{
			if ((i & mandatoryAttributes) != mandatoryAttributes)
				continue;
			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = baseDesc;
			auto inputElementDescs = ConstructInputLayout(i);
			desc.InputLayout = { inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()) };
			CComPtr<ID3D12PipelineState> pso;
			THROW_IF_FAILED(pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
			oPso[i] = pso;
		}
	}

#pragma endregion

#pragma region Internal

	void InitializeFX(ID3D12Device* pDevice)
	{
		THROW_IF_FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&gDxcUtils)));
		THROW_IF_FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&gDxcCompiler)));
		THROW_IF_FAILED(gDxcUtils->CreateDefaultIncludeHandler(&gDxcIncludeHandler));
	
		for (auto& [instanceId, info] : GetMaterialsRegistry())
			info.initializer(pDevice, info.material);
		for (auto& [instanceId, info] : GetMeshMaterialsRegistry())
			info.initializer(pDevice, info.material);
	}

	void ShutdonwnFX()
	{
		GetMaterialsRegistry().clear();
		GetMeshMaterialsRegistry().clear();
		gDxcIncludeHandler.Release();
		gDxcCompiler.Release();
		gDxcUtils.Release();
	}

#pragma endregion    

#pragma region Default initializers

	void MaterialDefaultInitialize(ID3D12Device* pDevice, const MaterialInitializationDesc& desc, Material& material)
	{
		std::vector<IntermediateCompiledCombination> compiledCombinations;
		CompileFxVariants(desc.fxDesc, compiledCombinations);

		for (const auto& combination : compiledCombinations)
		{
			ShaderFX currentFx;
			IntermediateCompiledReflection reflection;
			std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
			std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers;
			u32 rootIndex = 0u;
			CComPtr<ID3DBlob> signatureBlob;
			CComPtr<ID3DBlob> errorBlob;
			
			currentFx.vs = combination.vs;
			currentFx.ps = combination.ps;

			GetReflectionInfo(combination, reflection);

			for (const IntermediateShaderResource& resource : reflection.resources)
			{
				u64 fieldID = entt::hashed_string(resource.name, strlen(resource.name));
				
				// In case resource is a static sampler
				if (resource.type == D3D_SIT_SAMPLER && resource.bindCount == 1)
				{
					auto it = std::find_if(desc.staticSamplers.begin(), desc.staticSamplers.end(),
						[&](const StaticSamplerDesc& staticSampler)
						{
							return staticSampler.id == fieldID;
						});
					if (it != desc.staticSamplers.end())
					{
						CD3DX12_STATIC_SAMPLER_DESC staticSamplerDesc(resource.bindPoint, it->desc.Filter,
							it->desc.AddressU, it->desc.AddressV, it->desc.AddressW,
							it->desc.MipLODBias, it->desc.MaxAnisotropy, it->desc.ComparisonFunc,
							it->desc.BorderColor, it->desc.MinLOD, it->desc.MaxLOD,
							D3D12_SHADER_VISIBILITY_PIXEL);
						staticSamplers.push_back(staticSamplerDesc);
						continue;
					}
				}

				
				if (resource.type == D3D_SIT_CBUFFER)
				{
					// In case resource is a cbuffer
					CD3DX12_ROOT_PARAMETER rootParameter;
					// Check name prefix for "il" which means "inline" which means that this cbuffer sent as root constants
					if (strncmp(resource.name, "il", 2) == 0)
						rootParameter.InitAsConstants(resource.cbufferSize / sizeof(u32), resource.bindPoint, 0, D3D12_SHADER_VISIBILITY_ALL); // TODO num constants
					else
						rootParameter.InitAsConstantBufferView(resource.bindPoint, 0, D3D12_SHADER_VISIBILITY_ALL);
					rootParameters.push_back(rootParameter);
				}
				else
				{
					// In case resource is any other type (texture, buffer, sampler, etc.) no matter if it is special binding or not, create root parameter
					CD3DX12_ROOT_PARAMETER rootParameter;
					InitializeRootParameter(resource, rootParameter);
					rootParameters.push_back(rootParameter);
				}


				// In case resource is a environment variable (special binding)
				if (ProcessSpecialBindings(resource, rootIndex, fieldID, currentFx.specialBindings))
				{
					rootIndex++;
					continue;
				}

				// In case resource is a regular binding
				currentFx.bindings.push_back(ShaderResourceBinding
				{
					.type = resource.type,
					.fieldID = fieldID,
					.rootParamIndex = rootIndex
				});

				rootIndex++;
			}

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(static_cast<UINT>(rootParameters.size()),
				rootParameters.data(),
				staticSamplers.size(),
				staticSamplers.data(),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			THROW_IF_FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
			if (errorBlob)
			{
				std::string errorMsg((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
				THROW_ENGINE_EXCEPTION("Failed to serialize root signature: {}", errorMsg);
			}

			currentFx.rootSigBlob = signatureBlob;

			THROW_IF_FAILED(pDevice->CreateRootSignature(0,
				signatureBlob->GetBufferPointer(),
				signatureBlob->GetBufferSize(),
				IID_PPV_ARGS(&currentFx.rootSig)));

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = desc.basePsoDesc;
			psoDesc.pRootSignature = currentFx.rootSig.p;
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(currentFx.vs->GetBufferPointer(), currentFx.vs->GetBufferSize());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(currentFx.ps->GetBufferPointer(), currentFx.ps->GetBufferSize());

			THROW_IF_FAILED(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&currentFx.pso)));

			material.fx[combination.combinationId] = std::move(currentFx);
		}
	}

	TINYFX_API void MeshMaterialDefaultInitialize(ID3D12Device* pDevice, const MaterialInitializationDesc& desc, MeshMaterial& material)
	{
		std::vector<IntermediateCompiledCombination> compiledCombinations;
		CompileFxVariants(desc.fxDesc, compiledCombinations);

		for (const auto& combination : compiledCombinations)
		{
			MeshShaderFX currentFx;
			IntermediateCompiledReflection reflection;
			std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
			std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers;
			u32 rootIndex = 0u;
			CComPtr<ID3DBlob> signatureBlob;
			CComPtr<ID3DBlob> errorBlob;

			currentFx.vs = combination.vs;
			currentFx.ps = combination.ps;

			GetReflectionInfo(combination, reflection);

			for (const IntermediateShaderResource& resource : reflection.resources)
			{
				u64 fieldID = entt::hashed_string(resource.name, strlen(resource.name));

				// In case resource is a static sampler
				if (resource.type == D3D_SIT_SAMPLER && resource.bindCount == 1)
				{
					auto it = std::find_if(desc.staticSamplers.begin(), desc.staticSamplers.end(),
						[&](const StaticSamplerDesc& staticSampler)
						{
							return staticSampler.id == fieldID;
						});
					if (it != desc.staticSamplers.end())
					{
						CD3DX12_STATIC_SAMPLER_DESC staticSamplerDesc(resource.bindPoint, it->desc.Filter,
							it->desc.AddressU, it->desc.AddressV, it->desc.AddressW,
							it->desc.MipLODBias, it->desc.MaxAnisotropy, it->desc.ComparisonFunc,
							it->desc.BorderColor, it->desc.MinLOD, it->desc.MaxLOD,
							D3D12_SHADER_VISIBILITY_PIXEL);
						staticSamplers.push_back(staticSamplerDesc);
						continue;
					}
				}

				ShaderCBufferType cbufferType = ShaderCBufferType_CPU;
				if (resource.type == D3D_SIT_CBUFFER)
				{
					// In case resource is a cbuffer
					CD3DX12_ROOT_PARAMETER rootParameter;
					// Check name prefix for "il" which means "inline" which means that this cbuffer sent as root constants
					if (strncmp(resource.name, "il", 2) == 0)
					{
						rootParameter.InitAsConstants(/*resource.cbufferSize*/64 / sizeof(u32), resource.bindPoint, 0, D3D12_SHADER_VISIBILITY_ALL); // TODO num constants
						cbufferType = ShaderCBufferType_Constants;
					}
					else
						rootParameter.InitAsConstantBufferView(resource.bindPoint, 0, D3D12_SHADER_VISIBILITY_ALL);
					rootParameters.push_back(rootParameter);
				}
				else
				{
					// In case resource is any other type (texture, buffer, sampler, etc.) no matter if it is special binding or not, create root parameter
					CD3DX12_ROOT_PARAMETER rootParameter;
					InitializeRootParameter(resource, rootParameter);
					rootParameters.push_back(rootParameter);
				}


				// In case resource is a environment variable (special binding)
				if (ProcessSpecialBindings(resource, rootIndex, fieldID, currentFx.specialBindings))
				{
					rootIndex++;
					continue;
				}

				// In case resource is a regular binding
				currentFx.bindings.push_back(ShaderResourceBinding
					{
						.type = resource.type,
						.fieldID = fieldID,
						.rootParamIndex = rootIndex
					});

				rootIndex++;
			}

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(static_cast<UINT>(rootParameters.size()),
				rootParameters.data(),
				staticSamplers.size(),
				staticSamplers.data(),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			THROW_IF_FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
			if (errorBlob)
			{
				std::string errorMsg((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
				THROW_ENGINE_EXCEPTION("Failed to serialize root signature: {}", errorMsg);
			}

			currentFx.rootSigBlob = signatureBlob;

			THROW_IF_FAILED(pDevice->CreateRootSignature(0,
				signatureBlob->GetBufferPointer(),
				signatureBlob->GetBufferSize(),
				IID_PPV_ARGS(&currentFx.rootSig)));

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = desc.basePsoDesc;
			psoDesc.pRootSignature = currentFx.rootSig.p;
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(currentFx.vs->GetBufferPointer(), currentFx.vs->GetBufferSize());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(currentFx.ps->GetBufferPointer(), currentFx.ps->GetBufferSize());

			GenInputVariantsPSOs(psoDesc, pDevice, reflection.mandatoryInputAttributes, currentFx.pso);
			material.fx[combination.combinationId] = std::move(currentFx);
		}
	}

#pragma endregion
}