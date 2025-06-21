#pragma once
#include <d3d12.h>
#include <map>
#include "d3d-shader-loader-types.h"
#include "d3d-shader-loader-helper.h"
#include "d3d12-shader-loader-rootsig-library.h"
#include <filesystem>



struct D3DPipeline
{
	ID3D12PipelineState*	PipelineState;
	ID3D12StateObject*		StateObject;

	// Note: This struct does not own this
	ID3D12RootSignature*	RootSignature;

	// TODO: This should be checked against
	// some sort of authoritative source
	// so someone couldn't change the shaders
	// and give themselves wallhacks for example
	// by changing the shaders locally.
	uint64_t				FileHash;
};


class D3D12PipelineCache
{
public:

	// The constructor takes a ID3D12 1.0 Device, for max compatibility
	// Must NOT be null
	explicit D3D12PipelineCache(ID3D12Device* device);

	~D3D12PipelineCache();

	/*
	* @brief: Sets the print handler internally. Allows for users to set custom logging functionality.
	* By default prints to std::cout
	*/
	void SetPrintHandler(ID3DShaderLoaderPrintHandler* handler);

	/*
	* @brief: Given a directory of compiled shaders, load them and turn them into their
	* associated "ID3D12PipelineState*", "ID3D12RootSignature*", "ID3D12StateObject*" objects.
	* 
	* @param dirPath: A std::filesystem::path pointing to the directory you wish to load.
	* 
	* @param flags: Load the shaders compiled with the associated flags.
	*	WithDebugInfo_NoOptimization is compiled with "-Zi" unless overidden by the compiler
	*	WithoutDebugInfo_Optimize is compiled with "-O3" unless overridden by the compiler
	* 
	* @returns: false if unable to load, true if able to load
	*/
	bool LoadDirectory(const std::filesystem::path& dirPath, CompilerFlags flags);

	/*
	* @brief: Loads a given pipeline based on the associated shader's file name.
	* 
	* @param name: The source file name of the pipeline to load
	* @param outPipeline: The associated ID3D12* objects
	* 
	* @returns: false if not able to locate pipeline, true if able to.
	*/
	bool FindPipeline(const std::string& name, D3DPipeline* outPipeline);


private:

	void CheckRaytracingSupport();

	void BuildDxrStateDesc(const RAYTRACING_PIPELINE_STATE_DESC& desc, const std::string& shaderName, D3D12_STATE_OBJECT_DESC& outDesc);

	ID3D12Device* m_device;

	ID3DShaderLoaderPrintHandler* m_print;

	LoaderPriv::D3D12RootSignatureLibrary m_rootSigLib;

	std::map<std::string, D3DPipeline> m_library;

	bool m_hasRaytracingSupport;
};