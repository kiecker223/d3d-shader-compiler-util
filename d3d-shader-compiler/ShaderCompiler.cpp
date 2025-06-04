#include "ShaderCompiler.h"
#include "D3DCompilePath.h"


SHADER CompileVertexShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileVertexShader(InByteCode);
	}
	return {};
}

SHADER CompilePixelShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompilePixelShader(InByteCode);
	}
	return {};
}

SHADER CompileHullShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileHullShader(InByteCode);
	}
	return {};
}
SHADER CompileDomainShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileDomainShader(InByteCode);
	}
	return {};
}
SHADER CompileGeometryShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileGeometryShader(InByteCode);
	}
	return {};
}
SHADER CompileComputeShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileComputeShader(InByteCode);
	}
	return {};
}

SHADER CompileRaytracingShader(const std::string& InByteCode, ShaderCompilationType CompilationMode)
{
	if (CompilationMode == DXIL)
	{
		return D3DCompileRaytracingShader(InByteCode);
	}
	return {};
}