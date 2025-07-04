#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "Format.h"
#include "ShaderCompiler.h"
#include "nlohmann.hpp"


typedef enum ESHADER_PARAMETER_TYPE {
	SHADER_PARAMETER_TYPE_CBV,
	SHADER_PARAMETER_TYPE_SRV,
	SHADER_PARAMETER_TYPE_UAV
} ESHADER_PARAMETER_TYPE;

typedef enum EPOLYGON_TYPE {
	POLYGON_TYPE_POINTS,
	POLYGON_TYPE_LINES,
	POLYGON_TYPE_TRIANGLES,
	POLYGON_TYPE_TRIANGLE_STRIPS
} EPOLYGON_TYPE;

typedef enum ECOMPARISON_FUNCTION {
	COMPARISON_FUNCTION_NEVER,
	COMPARISON_FUNCTION_LESS,
	COMPARISON_FUNCTION_EQUAL,
	COMPARISON_FUNCTION_LESS_EQUAL,
	COMPARISON_FUNCTION_GREATER,
	COMPARISON_FUNCTION_NOT_EQUAL,
	COMPARISON_FUNCTION_GREATER_EQUAL,
	COMPARISON_FUNCTION_ALWAYS
} ECOMPARISON_FUNCTION;

typedef enum ESTENCIL_OP {
	STENCIL_OP_KEEP,
	STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INCR_SAT,
	STENCIL_OP_DECR_SAT,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCR,
	STENCIL_OP_DECR
} ESTENCIL_OP;

typedef enum EBLEND_STYLE {
	BLEND_STYLE_ZERO,
	BLEND_STYLE_ONE,
	BLEND_STYLE_SRC_COLOR,
	BLEND_STYLE_INV_SRC_COLOR,
	BLEND_STYLE_SRC_ALPHA,
	BLEND_STYLE_INV_SRC_ALPHA,
	BLEND_STYLE_DEST_ALPHA,
	BLEND_STYLE_INV_DEST_ALPHA,
	BLEND_STYLE_DEST_COLOR,
	BLEND_STYLE_INV_DEST_COLOR,
	BLEND_STYLE_SRC_ALPHA_SAT,
	BLEND_STYLE_BLEND_FACTOR,
	BLEND_STYLE_INV_BLEND_FACTOR,
	BLEND_STYLE_SRC1_COLOR,
	BLEND_STYLE_INV_SRC1_COLOR,
	BLEND_STYLE_SRC1_ALPHA,
	BLEND_STYLE_INV_SRC1_ALPHA
} EBLEND_STYLE;

typedef enum EBLEND_OP {
	BLEND_OP_ADD,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
} EBLEND_OP;

typedef enum ELOGIC_OP {
	LOGIC_OP_CLEAR,
	LOGIC_OP_SET,
	LOGIC_OP_COPY,
	LOGIC_OP_COPY_INVERTED,
	LOGIC_OP_NOOP,
	LOGIC_OP_INVERT,
	LOGIC_OP_AND,
	LOGIC_OP_NAND,
	LOGIC_OP_OR,
	LOGIC_OP_NOR,
	LOGIC_OP_XOR,
	LOGIC_OP_EQUIV,
	LOGIC_OP_AND_REVERSE,
	LOGIC_OP_AND_INVERTED,
	LOGIC_OP_OR_REVERSE,
	LOGIC_OP_OR_INVERTED
} ELOGIC_OP;

typedef enum EMULTISAMPLE_LEVEL {
	MULTISAMPLE_LEVEL_0,
	MULTISAMPLE_LEVEL_4X,
	MULTISAMPLE_LEVEL_8X,
	MULTISAMPLE_LEVEL_16X
} EMULTISAMPLE_LEVEL;

enum {
	ENABLE_ALL_COLOR_WRITE = 15,
};

typedef enum EINITDEFAULT {
	INIT_DEFAULT
} EINITDEFAULT;

typedef struct GFX_DEPTH_STENCIL_OP_DESC {
	ESTENCIL_OP StencilFailOp;
	ESTENCIL_OP StencilDepthFailOp;
	ESTENCIL_OP StencilPassOp;
	ECOMPARISON_FUNCTION ComparisonFunction;
} GFX_DEPTH_STENCIL_OP_DESC;

typedef struct GFX_DEPTH_STENCIL_DESC {
	EFORMAT Format;
	bool bDepthEnable;
	uint32_t DepthWriteMask;
	ECOMPARISON_FUNCTION DepthFunction;
	bool bStencilEnable;
	GFX_DEPTH_STENCIL_OP_DESC FrontFace;
	GFX_DEPTH_STENCIL_OP_DESC BackFace;
} GFX_DEPTH_STENCIL_DESC;

typedef struct GFX_RENDER_TARGET_DESC {
	bool bBlendEnable;
	bool bLogicOpEnable;
	EBLEND_STYLE SrcBlend;
	EBLEND_STYLE DstBlend;
	EBLEND_OP BlendOp;
	EBLEND_STYLE SrcBlendAlpha;
	EBLEND_STYLE DstBlendAlpha;
	EBLEND_OP AlphaBlendOp;
	ELOGIC_OP LogicOp;
	EFORMAT Format;
} GFX_RENDER_TARGET_DESC;

typedef enum EINPUT_ITEM_FORMAT {
	INPUT_ITEM_FORMAT_FLOAT,
	INPUT_ITEM_FORMAT_INT,
	INPUT_ITEM_FORMAT_FLOAT2,
	INPUT_ITEM_FORMAT_INT2,
	INPUT_ITEM_FORMAT_FLOAT3,
	INPUT_ITEM_FORMAT_INT3,
	INPUT_ITEM_FORMAT_FLOAT4,
	INPUT_ITEM_FORMAT_INT4,
	INPUT_ITEM_FORMAT_INVALID
} EINPUT_ITEM_FORMAT;

inline uint32_t TranslateItemFormatSize(EINPUT_ITEM_FORMAT Format)
{
	switch (Format)
	{
	case INPUT_ITEM_FORMAT_FLOAT:
	case INPUT_ITEM_FORMAT_INT:
		return 4;
	case INPUT_ITEM_FORMAT_FLOAT2:
	case INPUT_ITEM_FORMAT_INT2:
		return 8;
	case INPUT_ITEM_FORMAT_FLOAT3:
	case INPUT_ITEM_FORMAT_INT3:
		return 12;
	case INPUT_ITEM_FORMAT_FLOAT4:
	case INPUT_ITEM_FORMAT_INT4:
		return 16;
	}
	return 0;
}

typedef struct GFX_INPUT_ITEM_DESC {
	std::string Name;
	EINPUT_ITEM_FORMAT ItemFormat;
} GFX_INPUT_ITEM_DESC;

typedef struct GFX_INPUT_LAYOUT_DESC {
	std::vector<GFX_INPUT_ITEM_DESC> InputItems;
} GFX_INPUT_LAYOUT_DESC;


typedef struct GFX_RASTER_DESC {
	bool bFillSolid;
	bool bCull;
	bool bIsCounterClockwiseForward;
	bool bDepthClipEnable;
	bool bAntialiasedLineEnabled;
	bool bMultisampleEnable;
	float DepthBiasClamp;
	float SlopeScaledDepthBias;
	EMULTISAMPLE_LEVEL MultisampleLevel;
} GFX_RASTER_DESC;



inline GFX_DEPTH_STENCIL_DESC CreateDefaultGFXDepthStencilDesc()
{
	GFX_DEPTH_STENCIL_DESC Result = { };
	Result.Format = FORMAT_D24_UNORM_S8_UINT;
	Result.bDepthEnable = true;
	Result.DepthWriteMask = 0xffffffff;
	Result.DepthFunction = COMPARISON_FUNCTION_LESS;
	Result.bStencilEnable = false;
	GFX_DEPTH_STENCIL_OP_DESC Desc = { STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP, COMPARISON_FUNCTION_LESS };
	Result.FrontFace = Desc;
	Result.BackFace = Desc;
	return Result;
}

inline GFX_RENDER_TARGET_DESC CreateDefaultGFXRenderTargetDesc()
{
	GFX_RENDER_TARGET_DESC Result = { };
	Result.bBlendEnable = false;
	Result.bLogicOpEnable = false;
	Result.SrcBlend = BLEND_STYLE_ONE;
	Result.DstBlend = BLEND_STYLE_ZERO;
	Result.BlendOp = BLEND_OP_ADD;
	Result.SrcBlendAlpha = BLEND_STYLE_ONE;
	Result.DstBlendAlpha = BLEND_STYLE_ZERO;
	Result.AlphaBlendOp = BLEND_OP_ADD;
	Result.LogicOp = LOGIC_OP_NOOP;
	Result.Format = FORMAT_R8G8B8A8_UNORM;
	return Result;
}

typedef struct PIPELINE_RESOURCE_COUNTERS {
	uint8_t NumConstantBuffers;
	uint8_t NumShaderResourceViews;
	uint8_t NumUnorderedAccessViews;
	uint8_t NumSamplers;
} PIPELINE_RESOURCE_COUNTERS;

typedef struct FULL_PIPELINE_DESCRIPTOR {
	PIPELINE_RESOURCE_COUNTERS Counts;
	SHADER VS;
	SHADER PS;
	SHADER HS;
	SHADER DS;
	SHADER GS;
	GFX_INPUT_LAYOUT_DESC InputLayout;
	EPOLYGON_TYPE PolygonType;
	GFX_RASTER_DESC RasterDesc;
	bool bEnableAlphaToCoverage;
	bool bIndependentBlendEnable;
	GFX_RENDER_TARGET_DESC RtvDescs[8];
	GFX_DEPTH_STENCIL_DESC DepthStencilState;
	uint32_t NumRenderTargets;
} FULL_PIPELINE_DESCRIPTOR;

typedef struct COMPUTE_PIPELINE_DESC {
	PIPELINE_RESOURCE_COUNTERS Counts;
	SHADER CS;
} COMPUTE_PIPELINE_DESC;

typedef struct RAYTRACING_HIT_GROUP_DESC {
	std::string ClosestHit;
	std::string AnyHit;
	std::string ExportName;
} RAYTRACING_HIT_GROUP_DESC;

typedef struct RAYTRACING_PIPELINE_DESC {
	PIPELINE_RESOURCE_COUNTERS				Counts;
	SHADER									Library;
	uint32_t								PayloadSizeInBytes;
	uint32_t								MaxRaytraceRecurseDepth;
	std::vector<RAYTRACING_HIT_GROUP_DESC>	HitGroups;
} RAYTRACING_PIPELINE_DESC;

inline bool HasGeometryShader(const FULL_PIPELINE_DESCRIPTOR& Desc)
{
	return Desc.GS.WasCompiled;
}

inline bool HasHullShader(const FULL_PIPELINE_DESCRIPTOR& Desc)
{
	return Desc.HS.WasCompiled;
}

inline bool HasDomainShader(const FULL_PIPELINE_DESCRIPTOR& Desc)
{
	return Desc.DS.WasCompiled;
}

GFX_RASTER_DESC CreateDefaultGFXRasterDesc();

FULL_PIPELINE_DESCRIPTOR CreateDefaultDescriptor();

void GraphicsPipelineToJson(const FULL_PIPELINE_DESCRIPTOR& desc, nlohmann::json& outJson);

void RaytracingPipelineToJson(const RAYTRACING_PIPELINE_DESC& desc, nlohmann::json& outJson);

void ComputePipelineToJson(const COMPUTE_PIPELINE_DESC& desc, nlohmann::json& outJson);
