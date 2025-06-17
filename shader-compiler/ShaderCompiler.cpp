#include "ShaderCompiler.h"
#include "D3DCompilePath.h"
#include <d3dcommon.h>
#include <dxc/dxcapi.h>
#include <iostream>

#define failed(x) ((x) < 0)





//ShaderCompile(InByteCode, L"VSMain", "vs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"PSMain", "ps_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"HSMain", "hs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"DSMain", "ds_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"GSMain", "gs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, L"main", "cs_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);
//ShaderCompile(InByteCode, nullptr, "lib_6_3", (CompilerFlags)Flags, &Result.DXILStages[Flags]);


bool ShaderCompiler::InitializeDxcResources()
{
	if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils))))
	{
		return false;
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler))))
	{
		return false;
	}

	// Check required args are set
	if (m_Model == "")
	{
		std::cout << "[ERROR]: Shader model needs to be set before setting up dxc resources" << std::endl;
		return false;
	}

	m_Utils->BuildArguments(nullptr, )

	/*
	case CompilerFlags::WithDebugInfo_NoOptimization: {
		LPCWSTR CFlags[] = { L"-Zi" };
		if (FAILED(Utils->BuildArguments(nullptr, pEntry, WModel.c_str(), CFlags, 1, nullptr, 0, &Args)))
		{
			std::cout << "Failed to build arguments?" << std::endl;
			return (HRESULT)-1;
		}
	} break;
	case CompilerFlags::WithoutDebugInfo_Optimize: {
		LPCWSTR CFlags[] = { L"-O1" };
		if (FAILED(Utils->BuildArguments(nullptr, pEntry, WModel.c_str(), CFlags, 1, nullptr, 0, &Args)))
		{
			std::cout << "Failed to build arguments?" << std::endl;
			return (HRESULT)-1;
		}
	} break;
	*/

	//m_D3DArgs[CompilerFlags::WithDebugInfo_NoOptimization];

	

}

void ShaderCompiler::SetShaderModel(const std::string& ShaderModel)
{
}

void ShaderCompiler::SetVulkanExtraFlags(const std::string& ExtraFlags)
{
}

void ShaderCompiler::SetD3DExtraFlags(const std::string& ExtraFlags)
{
}

bool ShaderCompiler::CompileVertexShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompilePixelShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompileHullShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompileDomainShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompileGeometryShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompileComputeShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

bool ShaderCompiler::CompileRaytracingShader(const std::string& InByteCode, SHADER* shader)
{
	return false;
}

uint32_t ShaderCompiler::ShaderCompile(
	const std::string& SourceFile,
	const wchar_t* pEntry,
	const std::string& Model,
	CompilerFlags Flags,
	ShaderCompilationType Type,
	SHADER_BYTECODE* OutByteCode
) {
	ComPtr<IDxcCompilerArgs> Args = nullptr;
	ComPtr<IDxcResult> Result = nullptr;
	HRESULT CompileRes = (HRESULT)0;

	DxcBuffer Buf = { };
	Buf.Encoding = DXC_CP_UTF8;
	Buf.Ptr = SourceFile.c_str();
	Buf.Size = SourceFile.length();

	std::wstring WModel(Model.begin(), Model.end());
	switch (Flags)
	{
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

	// So I unintentinally programmed in this behavior, but
	// for some godforsaken reason, when you call Compile earlier, it doesn't fail
	// if the shader doesn't compile. Why? I have no idea.
	// But if this ends up giving you no output then you know the shader didn't compile
	// and the reason that the compiler couldn't compile it has been printed before.
	// So this sorta works even though it sucks.
	ComPtr<IDxcBlob> ShaderCode = nullptr;
	Result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderCode), nullptr);

	OutByteCode->ByteCode.resize(ShaderCode->GetBufferSize());
	memcpy(OutByteCode->ByteCode.data(), ShaderCode->GetBufferPointer(), ShaderCode->GetBufferSize());

	ShaderCode->Release();

	return CompileRes;
}
