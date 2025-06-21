#include "ShaderCompiler.h"
#include "D3DCompilePath.h"
#include <d3dcommon.h>
#include <dxc/dxcapi.h>
#include <iostream>
#include <regex>

#define failed(x) ((x) < 0)





//ShaderCompile(InByteCode, L"VSMain", "vs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"PSMain", "ps_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"HSMain", "hs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"DSMain", "ds_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"GSMain", "gs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"main", "cs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, nullptr, "lib_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);


// Its assumed before we get here "model" is validated already
static void ModelToNum(const std::wstring& model, uint32_t& major, uint32_t& minor)
{
	std::wstringstream str(model);

	std::wstring majorStr;
	std::wstring minorStr;

	std::getline(str, majorStr, L'_');
	std::getline(str, minorStr, L'_');

	major = std::wcstoul(majorStr.c_str(), nullptr, 10);
	minor = std::wcstoul(minorStr.c_str(), nullptr, 10);
}

static void SplitArgs(std::vector<std::wstring>& outArgs, const std::string& args)
{
	std::wstring wArgs(args.begin(), args.end());
	std::wstringstream str(wArgs);
	std::wstring buf;

	while (std::getline(str, buf, L' '))
	{
		if (buf == L"")
		{
			outArgs.push_back(buf);
		}
	}
}

static std::wstring StageToModel(ShaderStages stage, std::wstring model)
{
	switch (stage)
	{
	case STAGE_VERTEX: return L"vs_" + model;
	case STAGE_HULL: return L"hs_" + model;
	case STAGE_DOMAIN: return L"ds_" + model;
	case STAGE_GEOMETRY: return L"gs_" + model;
	case STAGE_PIXEL: return L"ps_" + model;
	case STAGE_COMPUTE: return L"cs_" + model;
	case STAGE_RAYTRACING: {
		uint32_t major = 0;
		uint32_t minor = 0;
		ModelToNum(model, major, minor);

		if (major < 6)
		{
			return L"";
		}

		if (minor < 3)
		{
			return L"";
		}

		return L"lib_" + model;
	}
	}
	return L"";
}

static std::wstring StageToEntry(ShaderStages stage)
{
	switch (stage)
	{
	case STAGE_VERTEX: return std::wstring(VertexEntry.begin(), VertexEntry.end());
	case STAGE_HULL: return std::wstring(HullEntry.begin(), HullEntry.end());
	case STAGE_DOMAIN: return std::wstring(DomainEntry.begin(), DomainEntry.end());
	case STAGE_GEOMETRY: return std::wstring(GeometryEntry.begin(), GeometryEntry.end());
	case STAGE_PIXEL: return std::wstring(PixelEntry.begin(), PixelEntry.end());
	case STAGE_COMPUTE: return std::wstring(ComputeEntry.begin(), ComputeEntry.end());
	}
	return L"";
}


ShaderCompilerIncludeHandler::ShaderCompilerIncludeHandler(const std::filesystem::path& startPath) :
	m_RefCount(1)
{
}

ShaderCompilerIncludeHandler::~ShaderCompilerIncludeHandler()
{
}

HRESULT __stdcall ShaderCompilerIncludeHandler::QueryInterface(REFIID riid, void** ppvObject)
{
	(void)riid;
	(void)ppvObject;
	return E_FAIL;
}

ULONG __stdcall ShaderCompilerIncludeHandler::AddRef()
{
	m_RefCount++;
	return m_RefCount;
}

ULONG __stdcall ShaderCompilerIncludeHandler::Release()
{
	if (m_RefCount == 0)
	{
		return 0;
	}
	
	m_RefCount--;
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT __stdcall ShaderCompilerIncludeHandler::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
{
	wchar_t fullPath[4096] = { 0 };
	swprintf(fullPath, L"%s/%s", m_Cwd.c_str(), pFilename);

	return m_DefaultHandler->LoadSource((LPCWSTR)fullPath, ppIncludeSource);
}


ShaderCompiler::ShaderCompiler(std::filesystem::path shaderDir) :
	m_ArgsSetup(false)
{
	ShaderCompilerIncludeHandler* handler = new ShaderCompilerIncludeHandler(shaderDir);
	m_IncludeHandler.UnsafeSet(handler);
}

bool ShaderCompiler::InitializeDxcResources()
{
	if (failed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils))))
	{
		return false;
	}
	if (failed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler))))
	{
		return false;
	}

	ComPtr<IDxcIncludeHandler> includeHandler;
	m_Utils->CreateDefaultIncludeHandler(&includeHandler);

	m_IncludeHandler->SetDefaultHandler(includeHandler);

	return true;
}

bool ShaderCompiler::SetupArgs()
{
	if (m_ArgsSetup)
	{
		return true;
	}

	if (m_Utils.Ptr == nullptr || m_Compiler.Ptr == nullptr)
	{
		return false;
	}


	for (uint32_t x = 0; x < SHADER_COMPILATION_TYPE_NUM; x++)
	{
		for (uint32_t y = 0; y < COMPILER_FLAGS_NUM; y++)
		{
			std::vector<std::wstring> args;

			bool skipSetup = false;
			if (x == DXIL)
			{
				if (m_D3DOverride[y].size() != 0)
				{
					skipSetup = true;
					args = m_D3DOverride[y];
				}
			}
			else if (x == SPIRV)
			{
				if (m_VulkanOverride[y].size() != 0)
				{
					skipSetup = true;
					args = m_VulkanOverride[y];
				}
			}
			if (!skipSetup)
			{
				if (y == CompilerFlags::WithDebugInfo_NoOptimization)
				{
					args.push_back(L"-Zi");
				}
				else if (y == CompilerFlags::WithoutDebugInfo_Optimize)
				{
					args.push_back(L"-O3");
				}

				if (x == SPIRV)
				{
					args.push_back(L"-spirv");
				}
			}

			for (uint32_t i = 0; i < STAGE_NUM; i++)
			{
				std::wstring model = StageToModel((ShaderStages)i, m_Model);
				std::wstring entry = StageToEntry((ShaderStages)i);

				std::vector<LPCWSTR> actualArgs;
				for (std::wstring& arg : args)
				{
					actualArgs.push_back(arg.c_str());
				}
				ComPtr<IDxcCompilerArgs> outArgs;
				if (failed(m_Utils->BuildArguments(
					nullptr,
					entry.c_str(),
					model.c_str(),
					actualArgs.data(),
					actualArgs.size(),
					nullptr,
					0,
					&outArgs))) 
				{
					std::cout << "[ERROR]: Failed to build arguments for shader" << std::endl;
					return false;
				}

				if (x == DXIL)
				{
					m_D3DArgs[y][i] = outArgs;
				}
				else if (x == SPIRV)
				{
					m_VKArgs[y][i] = outArgs;
				}
			}
		}
	}

	return true;
}

bool ShaderCompiler::SetShaderModel(const std::string& ShaderModel)
{
	std::regex r = std::regex("^[0-9]{1,2}_[0-9]{1,2}$");
	if (!std::regex_match(ShaderModel, r))
	{
		return false;
	}

	m_Model = std::wstring(ShaderModel.begin(), ShaderModel.end());
	return true;
}

void ShaderCompiler::SetVulkanExtraFlags(const std::string& ExtraFlags)
{
	std::stringstream str(ExtraFlags);
	std::string buf;

	while (std::getline(str, buf, ' '))
	{
		m_VKExtra.push_back(std::wstring(buf.begin(), buf.end()));
	}
}

void ShaderCompiler::SetD3DExtraFlags(const std::string& ExtraFlags)
{
	std::stringstream str(ExtraFlags);
	std::string buf;

	while (std::getline(str, buf, ' '))
	{
		m_D3DExtra.push_back(std::wstring(buf.begin(), buf.end()));
	}
}

void ShaderCompiler::SetVulkanOverrideFlags(CompilerFlags flags, const std::string& compilerFlagsOverride)
{
	std::stringstream str(compilerFlagsOverride);
	std::string buf;

	while (std::getline(str, buf, ' '))
	{
		m_VulkanOverride[flags].push_back(std::wstring(buf.begin(), buf.end()));
	}
}

void ShaderCompiler::SetD3DOverrideFlags(CompilerFlags flags, const std::string& compilerFlagsOverride)
{
	std::stringstream str(compilerFlagsOverride);
	std::string buf;

	while (std::getline(str, buf, ' '))
	{
		m_D3DOverride[flags].push_back(std::wstring(buf.begin(), buf.end()));
	}
}

bool ShaderCompiler::CompileVertexShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_VERTEX,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) || 
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_VERTEX,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_VERTEX,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_VERTEX,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompilePixelShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_PIXEL,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_PIXEL,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_PIXEL,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_PIXEL,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompileHullShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_HULL,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_HULL,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_HULL,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_HULL,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompileDomainShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_DOMAIN,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_DOMAIN,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_DOMAIN,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_DOMAIN,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompileGeometryShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_GEOMETRY,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_GEOMETRY,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_GEOMETRY,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_GEOMETRY,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompileComputeShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_COMPUTE,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_COMPUTE,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_COMPUTE,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_COMPUTE,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

bool ShaderCompiler::CompileRaytracingShader(const std::string& InByteCode, SHADER* shader)
{
	if (!shader)
	{
		return false;
	}
	if (failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			DXIL,
			STAGE_RAYTRACING,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			DXIL,
			STAGE_RAYTRACING,
			&shader->DXILStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithDebugInfo_NoOptimization,
			SPIRV,
			STAGE_RAYTRACING,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])) ||
		failed(ShaderCompile(
			InByteCode,
			WithoutDebugInfo_Optimize,
			SPIRV,
			STAGE_RAYTRACING,
			&shader->SPRVStages[WithDebugInfo_NoOptimization])))
	{
		std::cout << "[ERROR] Shader failed to compile" << std::endl;
		return false;
	}
	return true;
}

uint32_t ShaderCompiler::ShaderCompile(
	const std::string& SourceFile,
	CompilerFlags Flags,
	ShaderCompilationType Type,
	ShaderStages stage,
	SHADER_BYTECODE* OutByteCode
) {
	ComPtr<IDxcResult> Result = nullptr;
	HRESULT CompileRes = (HRESULT)0;

	DxcBuffer Buf = { };
	Buf.Encoding = DXC_CP_UTF8;
	Buf.Ptr = SourceFile.c_str();
	Buf.Size = SourceFile.length();

	ComPtr<IDxcCompilerArgs> Args;
	if (Type == DXIL)
	{
		Args = m_D3DArgs[Flags][stage];
	}
	else if (Type == SPIRV)
	{
		Args = m_VKArgs[Flags][stage];
	}

	CompileRes = m_Compiler->Compile(&Buf, Args->GetArguments(), Args->GetCount(), nullptr, IID_PPV_ARGS(&Result));
	Args->Release();
	if (FAILED(CompileRes))
	{
		return CompileRes;
	}

	ComPtr<IDxcBlob> ResultBlob = nullptr;
	Result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ResultBlob), nullptr);
	if (ResultBlob != nullptr && ResultBlob->GetBufferSize() != 0)
	{
		std::cout << "Failed to compile" << std::endl;
		std::cout << (const char*)ResultBlob->GetBufferPointer() << std::endl;
	}

	HRESULT Status;
	Result->GetStatus(&Status);

	if (!SUCCEEDED(Status))
	{
		return Status;
	}

	std::cout << "Successfully compiled" << std::endl;

	ComPtr<IDxcBlob> ShaderCode = nullptr;
	Result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderCode), nullptr);

	OutByteCode->ByteCode.resize(ShaderCode->GetBufferSize());
	memcpy(OutByteCode->ByteCode.data(), ShaderCode->GetBufferPointer(), ShaderCode->GetBufferSize());

	ShaderCode->Release();

	return CompileRes;
}
