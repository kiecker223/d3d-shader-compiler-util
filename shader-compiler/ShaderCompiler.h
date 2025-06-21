#pragma once
#include <string.h>
#include <vector>
#include "nlohmann.hpp"
#include "base64.hpp"
#include "ComPtr.h"
#include <d3dcommon.h>
#include <dxc/dxcapi.h>



enum ShaderCompilationType
{
	DXIL,
	SPIRV,
	SHADER_COMPILATION_TYPE_NUM
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
	COMPILER_FLAGS_NUM
};

enum ShaderStages
{
	STAGE_VERTEX,
	STAGE_HULL,
	STAGE_DOMAIN,
	STAGE_GEOMETRY,
	STAGE_PIXEL,
	STAGE_COMPUTE,
	STAGE_RAYTRACING,
	STAGE_NUM
};

inline std::string CompilerFlagsToStr(CompilerFlags flags)
{
	switch (flags)
	{
	case CompilerFlags::WithDebugInfo_NoOptimization:
		return "WithDebugInfo_NoOptimization";
	case CompilerFlags::WithoutDebugInfo_Optimize:
		return "WithoutDebugInfo_Optimize";
	}
	return "";
}

const std::string VertexEntry = "VSMain";
const std::string HullEntry = "HSMain";
const std::string DomainEntry = "DSMain";
const std::string GeometryEntry = "GSMain";
const std::string PixelEntry = "PSMain";
const std::string ComputeEntry = "CSMain";



typedef struct SHADER_BYTECODE {
	std::vector<uint8_t> ByteCode;
} SHADER_BYTECODE;

typedef struct SHADER {
	bool WasCompiled;
	SHADER_BYTECODE DXILStages[COMPILER_FLAGS_NUM];
	SHADER_BYTECODE SPRVStages[COMPILER_FLAGS_NUM];
} SHADER;


inline nlohmann::json SerializeShader(const SHADER* Shader)
{
	nlohmann::json Result;
	Result["SPRV"] = {
		"WithDebugInfo_NoOptimization", base64::to_base64(
			Shader->SPRVStages[CompilerFlags::WithDebugInfo_NoOptimization].ByteCode
		),
		"WithoutDebugInfo_Optimize", base64::to_base64(
			Shader->SPRVStages[CompilerFlags::WithoutDebugInfo_Optimize].ByteCode
		)
	};
	Result["DXIL"] = {
		"WithDebugInfo_NoOptimization", base64::to_base64(
			Shader->DXILStages[CompilerFlags::WithDebugInfo_NoOptimization].ByteCode
		),
		"WithoutDebugInfo_Optimize", base64::to_base64(
			Shader->DXILStages[CompilerFlags::WithoutDebugInfo_Optimize].ByteCode
		)
	};
	return Result;
}



class ShaderCompilerIncludeHandler : public IDxcIncludeHandler 
{
public:

	ShaderCompilerIncludeHandler() = delete;
	ShaderCompilerIncludeHandler(
		const std::filesystem::path& startPath);
	~ShaderCompilerIncludeHandler();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef() override;

	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource);

	inline void SetDefaultHandler(ComPtr<IDxcIncludeHandler> defaultHandler)
	{
		m_DefaultHandler = defaultHandler;
	}

private:

	std::filesystem::path m_Cwd;

	ComPtr<IDxcIncludeHandler> m_DefaultHandler;

	ULONG m_RefCount;
};

class ShaderCompiler
{
public:

	ShaderCompiler() = default;
	ShaderCompiler(std::filesystem::path shaderDir);
	~ShaderCompiler() = default;

	bool InitializeDxcResources();

	bool SetupArgs();

	bool SetShaderModel(const std::string& ShaderModel);

	void SetVulkanExtraFlags(const std::string& ExtraFlags);

	void SetD3DExtraFlags(const std::string& ExtraFlags);

	void SetVulkanOverrideFlags(CompilerFlags flags, const std::string& compilerFlagsOverride);

	void SetD3DOverrideFlags(CompilerFlags flags, const std::string& compilerFlagsOverride);

	bool CompileVertexShader(const std::string& InByteCode, SHADER* shader);

	bool CompilePixelShader(const std::string& InByteCode, SHADER* shader);

	bool CompileHullShader(const std::string& InByteCode, SHADER* shader);

	bool CompileDomainShader(const std::string& InByteCode, SHADER* shader);

	bool CompileGeometryShader(const std::string& InByteCode, SHADER* shader);

	bool CompileComputeShader(const std::string& InByteCode, SHADER* shader);

	bool CompileRaytracingShader(const std::string& InByteCode, SHADER* shader);

private:

	uint32_t ShaderCompile(
		const std::string& SourceFile,
		CompilerFlags Flags,
		ShaderCompilationType Type,
		ShaderStages stage,
		SHADER_BYTECODE* OutByteCode
	);

	std::wstring m_Model;

	// Extra compilation flags
	std::vector<std::wstring> m_D3DExtra;
	std::vector<std::wstring> m_VKExtra;

	// Override flags for compilation
	// These will *completely* override
	// all default and specified flags for compilation
	std::vector<std::wstring> m_VulkanOverride[COMPILER_FLAGS_NUM];
	std::vector<std::wstring> m_D3DOverride[COMPILER_FLAGS_NUM];

	bool m_ArgsSetup;

	ComPtr<IDxcUtils> m_Utils;
	ComPtr<IDxcCompiler3> m_Compiler;
	ComPtr<ShaderCompilerIncludeHandler> m_IncludeHandler;

	ComPtr<IDxcCompilerArgs> m_D3DArgs[COMPILER_FLAGS_NUM][STAGE_NUM];
	ComPtr<IDxcCompilerArgs> m_VKArgs[COMPILER_FLAGS_NUM][STAGE_NUM];

};

