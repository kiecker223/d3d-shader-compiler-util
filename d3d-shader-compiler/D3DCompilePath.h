#pragma once

#include <string>
#include "ShaderCompiler.h"


SHADER D3DCompileVertexShader(const std::string& InByteCode);

SHADER D3DCompilePixelShader(const std::string& InByteCode);

SHADER D3DCompileHullShader(const std::string& InByteCode);

SHADER D3DCompileDomainShader(const std::string& InByteCode);

SHADER D3DCompileGeometryShader(const std::string& InByteCode);

SHADER D3DCompileComputeShader(const std::string& InByteCode);

SHADER D3DCompileRaytracingShader(const std::string& InByteCode);