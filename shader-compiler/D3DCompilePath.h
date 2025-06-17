#pragma once

#include <string>
#include "ShaderCompiler.h"


bool D3DCompileVertexShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompilePixelShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompileHullShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompileDomainShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompileGeometryShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompileComputeShader(const std::string& InByteCode, SHADER& Result);
bool D3DCompileRaytracingShader(const std::string& InByteCode, SHADER& Result);