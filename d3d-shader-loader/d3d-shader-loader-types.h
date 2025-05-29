#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "d3d-shader-loader-format.h"


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

typedef std::vector<uint8_t> ShaderByteCode;

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

inline GFX_RASTER_DESC CreateDefaultGFXRasterDesc()
{
	GFX_RASTER_DESC Result;
	Result.bFillSolid = true;
	Result.bCull = true;
	Result.bIsCounterClockwiseForward = false;
	Result.bDepthClipEnable = true;
	Result.bAntialiasedLineEnabled = false;
	Result.bMultisampleEnable = false;
	Result.DepthBiasClamp = 0.0f;
	Result.SlopeScaledDepthBias = 0.0f;
	return Result;
}

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

typedef enum EINPUT_ITEM_FORMAT {
	INPUT_ITEM_FORMAT_FLOAT,
	INPUT_ITEM_FORMAT_INT,
	INPUT_ITEM_FORMAT_FLOAT2,
	INPUT_ITEM_FORMAT_INT2,
	INPUT_ITEM_FORMAT_FLOAT3,
	INPUT_ITEM_FORMAT_INT3,
	INPUT_ITEM_FORMAT_FLOAT4,
	INPUT_ITEM_FORMAT_INT4
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

inline uint32_t GetLayoutDescSize(const GFX_INPUT_LAYOUT_DESC& InDesc)
{
	uint32_t Result = 0;
	for (uint32_t i = 0; i < InDesc.InputItems.size(); i++)
	{
		Result += TranslateItemFormatSize(InDesc.InputItems[i].ItemFormat);
	}
	return Result;
}

typedef struct PIPELINE_STATE_RESOURCE_COUNTS {
	uint8_t NumConstantBuffers, NumShaderResourceViews, NumSamplers, NumUnorderedAccessViews;
} PIPELINE_STATE_RESOURCE_COUNTS;

typedef struct GFX_PIPELINE_STATE_DESC {
	PIPELINE_STATE_RESOURCE_COUNTS Counts = { };
	ShaderByteCode VS;
	ShaderByteCode PS;
	ShaderByteCode DS;
	ShaderByteCode HS;
	ShaderByteCode GS;
	GFX_INPUT_LAYOUT_DESC InputLayout;
	EPOLYGON_TYPE PolygonType = POLYGON_TYPE_TRIANGLES;
	GFX_RASTER_DESC RasterDesc = { };
	bool bEnableAlphaToCoverage = false;
	bool bIndependentBlendEnable = false;
	GFX_RENDER_TARGET_DESC RtvDescs[8] = { };
	GFX_DEPTH_STENCIL_DESC DepthStencilState = { };
	uint32_t NumRenderTargets = 0;

	GFX_PIPELINE_STATE_DESC()
	{
	}

	GFX_PIPELINE_STATE_DESC(EINITDEFAULT)
	{
		InitDefault();
	}

	void InitDefault()
	{
		PolygonType = POLYGON_TYPE_TRIANGLES;
		RasterDesc = CreateDefaultGFXRasterDesc();
		bEnableAlphaToCoverage = false;
		bIndependentBlendEnable = false;
		NumRenderTargets = 1;
		for (uint32_t i = 0; i < 8; i++)
		{
			RtvDescs[i] = CreateDefaultGFXRenderTargetDesc();
		}
	}

} GFX_PIPELINE_STATE_DESC;

inline GFX_PIPELINE_STATE_DESC CreateDefaultGFXPipeline()
{
	GFX_PIPELINE_STATE_DESC Result(INIT_DEFAULT);
	return Result;
}

typedef struct COMPUTE_PIPELINE_STATE_DESC {
	PIPELINE_STATE_RESOURCE_COUNTS Counts;
	ShaderByteCode CS;
} COMPUTE_PIPELINE_STATE_DESC;

typedef struct RAYTRACING_PIPELINE_STATE_DESC {
	PIPELINE_STATE_RESOURCE_COUNTS Counts;
	bool bHasIntersection;
	bool bHasClosestHit;
	bool bHasAnyHit;
	ShaderByteCode Library;
} RAYTRACING_PIPELINE_STATE_DESC;


/// ---------------
/// Texture stuff
/// ---------------

typedef enum EWRAP_MODE {
	WRAP_MODE_LOOP,
	WRAP_MODE_CLAMP,
	WRAP_MODE_MIRROR
} EWRAP_MODE;

typedef enum EMAG_FILTERING {
	MAG_FILTERING_POINT,
	MAG_FILTERING_LINEAR
} EMAG_FILTERING;

typedef enum EMIN_FILTERING {
	MIN_FILTERING_POINT,
	MIN_FILTERING_LINEAR,
	MIN_FILTERING_POINT_MIPMAP_POINT,
	MIN_FILTERING_POINT_MIPMAP_LINEAR,
	MIN_FILTERING_LINEAR_MIPMAP_POINT,
	MIN_FILTERING_LINEAR_MIPMAP_LINEAR,
} EMIN_FILTERING;

typedef enum EANISOTROPIC_FILTER_OVERRIDE {
	ANISOTROPIC_FILTER_OVERRIDE_NO_OVERRIDE = 0,
	ANISOTROPIC_FILTER_OVERRIDE_ANISOTROPIC,
	ANISOTROPIC_FILTER_OVERRIDE_MINIMUM_ANISOTROPIC,
	ANISOTROPIC_FILTER_OVERRIDE_MAXIMUM_ANISOTROPIC
} EANISOTROPIC_FILTER_OVERRIDE;


typedef struct SAMPLER_DESC {
	EWRAP_MODE UAddress;
	EWRAP_MODE VAddress;
	EWRAP_MODE WAddress;
	EMAG_FILTERING MagFilter;
	EMIN_FILTERING MinFilter;
	EANISOTROPIC_FILTER_OVERRIDE AnisotropicFiltering;
} SAMPLER_DESC;

inline SAMPLER_DESC CreateDefaultSamplerDesc()
{
	return {
		WRAP_MODE_CLAMP,
		WRAP_MODE_CLAMP,
		WRAP_MODE_CLAMP,
		(EMAG_FILTERING)0, (EMIN_FILTERING)0,
		ANISOTROPIC_FILTER_OVERRIDE_MAXIMUM_ANISOTROPIC
	};
}

namespace LoaderPriv {

	EINPUT_ITEM_FORMAT ParseItemFormat(const std::string& ItemFormatStr);

	bool ParseBool(std::string BoolStr);

	EBLEND_STYLE ParseBlendStyle(const std::string& BlendStyleStr);

	EFORMAT ParseFormat(const std::string& FormatStr);

	EBLEND_OP ParseBlendOp(const std::string& BlendOpStr);

	ELOGIC_OP ParseLogicOp(const std::string& LogicOpStr);

	ECOMPARISON_FUNCTION ParseComparisonFunction(const std::string& ComparisonFuncStr);

	ESTENCIL_OP ParseStencilOp(const std::string StencilOpStr);

	EMULTISAMPLE_LEVEL ParseMultisampleLevel(const std::string& MultisampleLevelStr);

	EPOLYGON_TYPE ParsePolygonType(const std::string& PolygonTypeStr);

};