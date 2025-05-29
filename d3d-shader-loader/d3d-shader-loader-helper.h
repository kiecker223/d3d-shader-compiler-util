#pragma once

#include <d3d12.h>
#include <nlohmann.hpp>
#include <filesystem>
#include "d3d-shader-loader-types.h"


/*
* To assist with debugging here's a print handler
* that a user of this library can override and create to
* get error messages/failure reasons back without explicitly
* relying on std::cout.
*
* If one is not set D3DPipelineCache will create a default one that
* prints to std::cout.
*/
class ID3DShaderLoaderPrintHandler
{
public:

	virtual void ErrorImpl(const char* message) = 0;
	virtual void WarnImpl(const char* message) = 0;
	virtual void MessageImpl(const char* message) = 0;

	void Error(const char* fmt, ...);
	void Warn(const char* fmt, ...);
	void Message(const char* fmt, ...);
};

// Private functions
namespace LoaderPriv {

	void SetPrintHandler(ID3DShaderLoaderPrintHandler* handler);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12_TranslateGfxDesc(const GFX_PIPELINE_STATE_DESC& desc);

	void FreeD3D12GraphicsPipelineDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

	D3D12_SAMPLER_DESC D3D12_TranslateSamplerDesc(const SAMPLER_DESC& InDesc);

	D3D12_COMPUTE_PIPELINE_STATE_DESC D3D12_TranslateCmptDesc(const COMPUTE_PIPELINE_STATE_DESC& desc);

	bool LoadCmptDescFromJson(const nlohmann::json& json, COMPUTE_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags);

	bool LoadRTDescFromJson(const nlohmann::json& json, RAYTRACING_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags);

	bool LoadGfxDescFromJson(const nlohmann::json& json, GFX_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags);

	bool LoadGfxRasterDescFromJson(const nlohmann::json& json, GFX_RASTER_DESC& desc);

	bool LoadGfxInputLayoutFromJson(const nlohmann::json& json, GFX_INPUT_LAYOUT_DESC& desc);

	// outDesc is expected to be an array of 8 GFX_RENDER_TARGET_DESCs
	bool LoadGfxRTDescFromJson(const nlohmann::json& json, GFX_RENDER_TARGET_DESC* outDesc, uint32_t numRenderTargets);

	bool LoadGfxDepthStencilDescFromJson(const nlohmann::json& json, GFX_DEPTH_STENCIL_DESC& desc);
}

