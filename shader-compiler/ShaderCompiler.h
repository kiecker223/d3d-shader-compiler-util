#pragma once
#include <string.h>
#include <vector>
#include "nlohmann.hpp"
#include "base64.hpp"
#include <d3dcommon.h>
#include <dxc/dxcapi.h>



enum ShaderCompilationType
{
	DXIL,
	SPIRV,
	NUM
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
	SHADER_BYTECODE DXILStages[CompilerFlags::NUM];
	SHADER_BYTECODE SPRVStages[CompilerFlags::NUM];
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

// We don't want explicit dependencies on windows only headers
// Some day I want this to run on linux.
// Therefore quick and dirty little thing
template<typename T>
class ComPtr
{
public:

	ComPtr() = delete;
	ComPtr(T* p) : Ptr(p) {}
	ComPtr(ComPtr<T>& rhs) : Ptr(rhs.Ptr)
	{
		if (Ptr)
		{
			Ptr->AddRef();
		}
	}
	~ComPtr()
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
	}

	inline ComPtr<T>& operator = (T* rhs)
	{
		if (Ptr != rhs)
		{
			if (Ptr)
			{
				Ptr->Release();
			}
			Ptr = rhs;
			if (Ptr)
			{
				Ptr->AddRef();
			}
		}
		return *this;
	}

	inline ComPtr<T>& operator=(const ComPtr<T>& rhs)
	{
		return operator=(rhs.Ptr); // Calls the T* assignment overload
	}

	inline T* operator ->()
	{
		return Ptr;
	}

	inline T** operator &()
	{
		return ReleaseAndGetAddressOf();
	}

	inline const T* operator ->() const
	{
		return Ptr;
	}

	inline operator T* ()
	{
		return Ptr;
	}

	inline operator const T* () const
	{
		return Ptr;
	}

	inline T** ReleaseAndGetAddressOf()
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
		return &Ptr;
	}


	T* Ptr;

};

class ShaderCompiler
{
public:

	ShaderCompiler();
	~ShaderCompiler();

	bool InitializeDxcResources();

	void SetShaderModel(const std::string& ShaderModel);

	void SetVulkanExtraFlags(const std::string& ExtraFlags);

	void SetD3DExtraFlags(const std::string& ExtraFlags);

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
		const wchar_t* pEntry,
		const std::string& Model,
		CompilerFlags Flags,
		ShaderCompilationType Type,
		SHADER_BYTECODE* OutByteCode
	);

	std::string m_Model;
	std::string m_D3DExtra;
	std::string m_VKExtra;

	ComPtr<IDxcUtils> m_Utils;
	ComPtr<IDxcCompiler3> m_Compiler;

	ComPtr<IDxcCompilerArgs> m_D3DArgs[CompilerFlags::NUM];
	ComPtr<IDxcCompilerArgs> m_VKArgs[CompilerFlags::NUM];

};

