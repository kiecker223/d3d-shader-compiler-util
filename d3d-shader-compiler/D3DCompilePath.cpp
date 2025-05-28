#include "D3DCompilePath.h"
#include <d3dcommon.h>
#include <dxc/dxcapi.h>
#include <iostream>

#define failed(x) ((x) < 0)


HRESULT ShaderCompile(const std::string& SourceFile, const wchar_t* pEntry, const std::string& Model, CompilerFlags Flags, SHADER_BYTECODE* OutByteCode)
{
	IDxcUtils* Utils = nullptr;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Utils));
	IDxcCompiler3* Compiler = nullptr;	
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
	IDxcCompilerArgs* Args = nullptr;
	IDxcResult* Result = nullptr;
	HRESULT CompileRes = (HRESULT)0;

	DxcBuffer Buf;
	Buf.Encoding = 0;
	Buf.Ptr = SourceFile.c_str();
	Buf.Size = SourceFile.length();

	std::wstring WModel(Model.begin(), Model.end());
	switch (Flags)
	{
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
	}

	CompileRes = Compiler->Compile(&Buf, Args->GetArguments(), Args->GetCount(), nullptr, IID_PPV_ARGS(&Result));
	Args->Release();
	if (FAILED(CompileRes))
	{
		return CompileRes;
	}

	IDxcBlob* ResultBlob = nullptr;
	Result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ResultBlob), nullptr);
	if (ResultBlob != nullptr && ResultBlob->GetBufferSize() != 0)
	{
		std::cout << "Failed to compile" << std::endl;
		std::cout << (const char*)ResultBlob->GetBufferPointer() << std::endl;
	}

	std::cout << "Successfully compiled" << std::endl;

	// So I unintentinally programmed in this behavior, but
	// for some godforsaken reason, when you call Compile earlier, it doesn't fail
	// if the shader doesn't compile. Why? I have no idea.
	// But if this ends up giving you no output then you know the shader didn't compile
	// and the reason that the compiler couldn't compile it has been printed before.
	// So this sorta works even though it sucks.
	IDxcBlob* ShaderCode = nullptr;
	Result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderCode), nullptr);

	OutByteCode->ByteCode.resize(ShaderCode->GetBufferSize());
	memcpy(OutByteCode->ByteCode.data(), ShaderCode->GetBufferPointer(), ShaderCode->GetBufferSize());

	ShaderCode->Release();

	return CompileRes;
}

SHADER D3DCompileVertexShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"VSMain", "vs_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompilePixelShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"PSMain", "ps_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompileHullShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"HSMain", "hs_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompileDomainShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"DSMain", "ds_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompileGeometryShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"GSMain", "gs_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompileComputeShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, L"main", "cs_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}

SHADER D3DCompileRaytracingShader(const std::string& InByteCode)
{
	SHADER Result;
	for (uint32_t Flags = 0; Flags < (uint32_t)CompilerFlags::NUM; Flags++)
	{
		ShaderCompile(InByteCode, nullptr, "lib_6_3", (CompilerFlags)Flags, &Result.CompiledStages[Flags]);
	}
	return Result;
}