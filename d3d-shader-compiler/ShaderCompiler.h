#pragma once
#include <string.h>
#include <vector>
#include "nlohmann.hpp"
#include "base64.hpp"

enum ShaderCompilationType
{
	DXIL,
	SPIRV
};

/*
* ANY CHANGES HERE NEED TO BE REFLECTED IN d3d-shader-loader/d3d-shader-loader-types.h
* Intentionally chose to not have a common library because I wanted all the code in
* d3d-shader-loader to be able to be dragged and dropped into any project.
*/
enum CompilerFlags
{
	WithDebugInfo_NoOptimization,
	WithoutDebugInfo_Optimize,
	NUM
};

inline std::string CompilerFlagsToStr(CompilerFlags flags)
{
	switch (flags)
	{
	case WithDebugInfo_NoOptimization:
		return "WithDebugInfo_NoOptimization";
	case WithoutDebugInfo_Optimize:
		return "WithoutDebugInfo_Optimize";
	}
	return "";
}


typedef struct SHADER_BYTECODE {
	std::vector<uint8_t> ByteCode;
} SHADER_BYTECODE;

typedef struct SHADER {
	SHADER_BYTECODE CompiledStages[CompilerFlags::NUM];
} SHADER;


inline nlohmann::json SerializeShader(const SHADER* Shader) 
{
	nlohmann::json Result;
	Result["WithDebugInfo_NoOptimization"] = base64::to_base64(Shader->CompiledStages[CompilerFlags::WithDebugInfo_NoOptimization].ByteCode);
	Result["WithoutDebugInfo_Optimize"] = base64::to_base64(Shader->CompiledStages[CompilerFlags::WithoutDebugInfo_Optimize].ByteCode);
	return Result;
}

SHADER CompileVertexShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompilePixelShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompileHullShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompileDomainShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompileGeometryShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompileComputeShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);

SHADER CompileRaytracingShader(const std::string& InByteCode, ShaderCompilationType CompilationMode);