#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "Utils.h"
#include "base64.hpp"
#include "nlohmann.hpp"
#include "ShaderCompiler.h"
#include "PipelineCompiler.h"
#include "Pipeline.h"
#include "GraphicsAST.h"
#include "RaytracingAST.h"
#include "ComputeAST.h"


/*
void SetCompilationMode(ShaderCompilationType Mode)
{
	CompilationMode = Mode;
}


typedef struct VARIABLE_INFO {
	std::string Name;
	std::string Type;
	std::string Semantic;
} VARIABLE_INFO;


static bool VarHasSemantic(const VARIABLE_INFO& InVar)
{
	return InVar.Semantic.size() > 0;
}

typedef struct STRUCT_INFO {
	std::string Name;
	std::vector<VARIABLE_INFO> Variables;
} STRUCT_INFO;

typedef struct FUNCTION_INFO {
	std::string ReturnType;
	std::string Name;
	std::string Semantic;
	std::vector<VARIABLE_INFO> Parameters;
} FUNCTION_INFO;

static std::string ParseToEndOfScope(const std::string& Str, size_t Loc);


static FUNCTION_INFO GetFunctionInfo(std::string FunctionDecl)
{
	FUNCTION_INFO Result;

	size_t CurrentLoc = 0;

	for (; FunctionDecl[CurrentLoc] == ' ' || FunctionDecl[CurrentLoc] == '\n'; CurrentLoc++) {}
	for (; FunctionDecl[CurrentLoc] != ' ' && FunctionDecl[CurrentLoc] != '\n'; CurrentLoc++)
	{
		Result.ReturnType.push_back(FunctionDecl[CurrentLoc]);
	}

	size_t EndName = FunctionDecl.find('(', 0);

	for (; CurrentLoc != EndName; CurrentLoc++)
	{
		char C = FunctionDecl[CurrentLoc];
		if (C != ' ' && C != '\n' && C != '{')
		{
			Result.Name.push_back(C);
		}
	}

	VARIABLE_INFO VarInfo;
#define RESET_VAR() VarInfo.Name.clear(); VarInfo.Type.clear(); VarInfo.Semantic.clear();

	CurrentLoc = FunctionDecl.find('(', CurrentLoc) + 1;
	size_t EndFuncLoc = FunctionDecl.find(')', 0) + 1;

	for (; CurrentLoc != EndFuncLoc; CurrentLoc++)
	{
		size_t EndVarDeclLoc = FunctionDecl.find(',', CurrentLoc);

		if (EndVarDeclLoc == std::string::npos)
		{
			EndVarDeclLoc = EndFuncLoc;
		}

		bool bHasSemantic = false;
		std::string Type;
		for (uint32_t Tries = 0; Tries < 2; Tries++)
		{
			for (; FunctionDecl[CurrentLoc] != ' ' && FunctionDecl[CurrentLoc] != '\n'; CurrentLoc++)
			{
				Type.push_back(FunctionDecl[CurrentLoc]);
			}
			// If we have the various.. idk what these called, but the variable markers
			// skip it, for what we're doing we don't need them
			if (Type == "in" || Type == "inout" || Type == "out")
			{
				for (; FunctionDecl[CurrentLoc] == ' ' || FunctionDecl[CurrentLoc] == '\n'; CurrentLoc++) {}
			}
			else
			{
				VarInfo.Type = Type;
				break;
			}
		}
		for (; FunctionDecl[CurrentLoc] == ' ' || FunctionDecl[CurrentLoc] == '\n'; CurrentLoc++) {}
		for (; FunctionDecl[CurrentLoc] != ' ' && FunctionDecl[CurrentLoc] != '\n' && FunctionDecl[CurrentLoc] != ')'; CurrentLoc++)
		{
			VarInfo.Name.push_back(FunctionDecl[CurrentLoc]);
		}
		for (size_t i = CurrentLoc; i != EndVarDeclLoc; i++)
		{
			char C = FunctionDecl[i];
			if (C == ':')
			{
				bHasSemantic = true;
				break;
			}
		}
		if (bHasSemantic)
		{
			for (; CurrentLoc != EndVarDeclLoc; CurrentLoc++)
			{
				char C = FunctionDecl[CurrentLoc];
				if (C != ' ' && C != ',' && C != ')')
				{
					VarInfo.Semantic.push_back(C);
				}
			}
			if (VarInfo.Semantic.find("SV_") != std::string::npos)
			{
			}
			else
			{
				Result.Parameters.push_back(VarInfo);
			}
			RESET_VAR();
		}
		else
		{
			Result.Parameters.push_back(VarInfo);
			RESET_VAR();
		}
	}

#undef RESET_VAR
	return Result;
}

static STRUCT_INFO GetStructInfo(std::string StructDeclaration)
{
	STRUCT_INFO Result;

	RemoveTabs(StructDeclaration);
	RemoveAllNonSpaceWhiteSpace(StructDeclaration);

	size_t CurrentLoc = StructDeclaration.find("struct", 0);
	CurrentLoc += 6;
	size_t EndName = StructDeclaration.find("{", 0);

	for (; CurrentLoc != EndName; CurrentLoc++)
	{
		char C = StructDeclaration[CurrentLoc];
		if (C != ' ' && C != '\n' && C != '{')
		{
			Result.Name.push_back(C);
		}
	}

	std::string Members = ParseToEndOfScope(StructDeclaration, CurrentLoc);
	std::vector<std::string> MemberList = SplitString(Members, ';');

	for (std::string& Member : MemberList)
	{
		if (IsWhiteSpace(Member))
		{
			continue;
		}

		VARIABLE_INFO VarInfo;
		uint32_t i = 0;
		for (i = 0; Member[i] == ' ' || Member[i] == '\n'; i++) {}
		for (; Member[i] != ' ' && Member[i] != '\n'; i++)
		{
			VarInfo.Type.push_back(Member[i]);
		}

		for (; Member[i] == ' ' || Member[i] == '\n' || Member[i] == '\t'; i++) {}

		for (; Member[i] != ' ' && Member[i] != '\n'; i++)
		{
			if (Member[i] != ';' && Member[i] != ':')
			{
				VarInfo.Name.push_back(Member[i]);
			}
		}
		size_t SemanticDeclLoc = Member.find(':', 0);
		if (SemanticDeclLoc < Member.length())
		{
			for (; Member[i] != ':'; i++) {}
			i++;
			for (; Member[i] == ' ' || Member[i] == '\n'; i++) {}
			for (; Member[i] != ' ' && Member[i] != '\n' && Member[i] != ';' && i < Member.length(); i++)
			{
				VarInfo.Semantic.push_back(Member[i]);
			}
		}
		Result.Variables.push_back(VarInfo);

	}
	return Result;
}

static EINPUT_ITEM_FORMAT TypeToItemFormat(std::string InType)
{
	if (InType == "float")
	{
		return INPUT_ITEM_FORMAT_FLOAT;
	}
	if (InType == "float2")
	{
		return INPUT_ITEM_FORMAT_FLOAT2;
	}
	if (InType == "float3")
	{
		return INPUT_ITEM_FORMAT_FLOAT3;
	}
	if (InType == "float4")
	{
		return INPUT_ITEM_FORMAT_FLOAT2;
	}
	if (InType == "int")
	{
		return INPUT_ITEM_FORMAT_INT;
	}
	if (InType == "int2")
	{
		return INPUT_ITEM_FORMAT_INT2;
	}
	if (InType == "int3")
	{
		return INPUT_ITEM_FORMAT_INT3;
	}
	if (InType == "int4")
	{
		return INPUT_ITEM_FORMAT_INT4;
	}
	return INPUT_ITEM_FORMAT_INVALID;
}

static bool ParseBool(std::string BoolStr)
{
	if (BoolStr == "true")
		return true;
	if (BoolStr == "false")
		return false;
	return false;
}

static EBLEND_STYLE ParseBlendStyle(const std::string& BlendStyleStr)
{
#define CHECK_BLEND_STYLE(x)  if (BlendStyleStr == #x) { return x; }

	CHECK_BLEND_STYLE(BLEND_STYLE_ZERO);
	CHECK_BLEND_STYLE(BLEND_STYLE_ONE);
	CHECK_BLEND_STYLE(BLEND_STYLE_SRC_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_SRC_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_SRC_ALPHA);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_SRC_ALPHA);
	CHECK_BLEND_STYLE(BLEND_STYLE_DEST_ALPHA);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_DEST_ALPHA);
	CHECK_BLEND_STYLE(BLEND_STYLE_DEST_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_DEST_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_SRC_ALPHA_SAT);
	CHECK_BLEND_STYLE(BLEND_STYLE_BLEND_FACTOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_BLEND_FACTOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_SRC1_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_SRC1_COLOR);
	CHECK_BLEND_STYLE(BLEND_STYLE_SRC1_ALPHA);
	CHECK_BLEND_STYLE(BLEND_STYLE_INV_SRC1_ALPHA);
	return (EBLEND_STYLE)0;

#undef CHECK_BLEND_STYLE
}

inline EFORMAT ParseFormat(const std::string& FormatStr)
{
#define CHECK_FORMAT(x) if (FormatStr == #x) { return x; }

	CHECK_FORMAT(FORMAT_UNKNOWN);
	CHECK_FORMAT(FORMAT_R32G32B32A32_TYPELESS);
	CHECK_FORMAT(FORMAT_R32G32B32A32_FLOAT);
	CHECK_FORMAT(FORMAT_R32G32B32A32_UINT);
	CHECK_FORMAT(FORMAT_R32G32B32A32_SINT);
	CHECK_FORMAT(FORMAT_R32G32B32_TYPELESS);
	CHECK_FORMAT(FORMAT_R32G32B32_FLOAT);
	CHECK_FORMAT(FORMAT_R32G32B32_UINT);
	CHECK_FORMAT(FORMAT_R32G32B32_SINT);
	CHECK_FORMAT(FORMAT_R16G16B16A16_TYPELESS);
	CHECK_FORMAT(FORMAT_R16G16B16A16_FLOAT);
	CHECK_FORMAT(FORMAT_R16G16B16A16_UNORM);
	CHECK_FORMAT(FORMAT_R16G16B16A16_UINT);
	CHECK_FORMAT(FORMAT_R16G16B16A16_SNORM);
	CHECK_FORMAT(FORMAT_R16G16B16A16_SINT);
	CHECK_FORMAT(FORMAT_R32G32_TYPELESS);
	CHECK_FORMAT(FORMAT_R32G32_FLOAT);
	CHECK_FORMAT(FORMAT_R32G32_UINT);
	CHECK_FORMAT(FORMAT_R32G32_SINT);
	CHECK_FORMAT(FORMAT_R32G8X24_TYPELESS);
	CHECK_FORMAT(FORMAT_D32_FLOAT_S8X24_UINT);
	CHECK_FORMAT(FORMAT_R32_FLOAT_X8X24_TYPELESS);
	CHECK_FORMAT(FORMAT_X32_TYPELESS_G8X24_UINT);
	CHECK_FORMAT(FORMAT_R10G10B10A2_TYPELESS);
	CHECK_FORMAT(FORMAT_R10G10B10A2_UNORM);
	CHECK_FORMAT(FORMAT_R10G10B10A2_UINT);
	CHECK_FORMAT(FORMAT_R11G11B10_FLOAT);
	CHECK_FORMAT(FORMAT_R8G8B8A8_TYPELESS);
	CHECK_FORMAT(FORMAT_R8G8B8A8_UNORM);
	CHECK_FORMAT(FORMAT_R8G8B8A8_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_R8G8B8A8_UINT);
	CHECK_FORMAT(FORMAT_R8G8B8A8_SNORM);
	CHECK_FORMAT(FORMAT_R8G8B8A8_SINT);
	CHECK_FORMAT(FORMAT_R16G16_TYPELESS);
	CHECK_FORMAT(FORMAT_R16G16_FLOAT);
	CHECK_FORMAT(FORMAT_R16G16_UNORM);
	CHECK_FORMAT(FORMAT_R16G16_UINT);
	CHECK_FORMAT(FORMAT_R16G16_SNORM);
	CHECK_FORMAT(FORMAT_R16G16_SINT);
	CHECK_FORMAT(FORMAT_R32_TYPELESS);
	CHECK_FORMAT(FORMAT_D32_FLOAT);
	CHECK_FORMAT(FORMAT_R32_FLOAT);
	CHECK_FORMAT(FORMAT_R32_UINT);
	CHECK_FORMAT(FORMAT_R32_SINT);
	CHECK_FORMAT(FORMAT_R24G8_TYPELESS);
	CHECK_FORMAT(FORMAT_D24_UNORM_S8_UINT);
	CHECK_FORMAT(FORMAT_R24_UNORM_X8_TYPELESS);
	CHECK_FORMAT(FORMAT_X24_TYPELESS_G8_UINT);
	CHECK_FORMAT(FORMAT_R8G8_TYPELESS);
	CHECK_FORMAT(FORMAT_R8G8_UNORM);
	CHECK_FORMAT(FORMAT_R8G8_UINT);
	CHECK_FORMAT(FORMAT_R8G8_SNORM);
	CHECK_FORMAT(FORMAT_R8G8_SINT);
	CHECK_FORMAT(FORMAT_R16_TYPELESS);
	CHECK_FORMAT(FORMAT_R16_FLOAT);
	CHECK_FORMAT(FORMAT_D16_UNORM);
	CHECK_FORMAT(FORMAT_R16_UNORM);
	CHECK_FORMAT(FORMAT_R16_UINT);
	CHECK_FORMAT(FORMAT_R16_SNORM);
	CHECK_FORMAT(FORMAT_R16_SINT);
	CHECK_FORMAT(FORMAT_R8_TYPELESS);
	CHECK_FORMAT(FORMAT_R8_UNORM);
	CHECK_FORMAT(FORMAT_R8_UINT);
	CHECK_FORMAT(FORMAT_R8_SNORM);
	CHECK_FORMAT(FORMAT_R8_SINT);
	CHECK_FORMAT(FORMAT_A8_UNORM);
	CHECK_FORMAT(FORMAT_R1_UNORM);
	CHECK_FORMAT(FORMAT_R9G9B9E5_SHAREDEXP);
	CHECK_FORMAT(FORMAT_R8G8_B8G8_UNORM);
	CHECK_FORMAT(FORMAT_G8R8_G8B8_UNORM);
	CHECK_FORMAT(FORMAT_BC1_TYPELESS);
	CHECK_FORMAT(FORMAT_BC1_UNORM);
	CHECK_FORMAT(FORMAT_BC1_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_BC2_TYPELESS);
	CHECK_FORMAT(FORMAT_BC2_UNORM);
	CHECK_FORMAT(FORMAT_BC2_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_BC3_TYPELESS);
	CHECK_FORMAT(FORMAT_BC3_UNORM);
	CHECK_FORMAT(FORMAT_BC3_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_BC4_TYPELESS);
	CHECK_FORMAT(FORMAT_BC4_UNORM);
	CHECK_FORMAT(FORMAT_BC4_SNORM);
	CHECK_FORMAT(FORMAT_BC5_TYPELESS);
	CHECK_FORMAT(FORMAT_BC5_UNORM);
	CHECK_FORMAT(FORMAT_BC5_SNORM);
	CHECK_FORMAT(FORMAT_B5G6R5_UNORM);
	CHECK_FORMAT(FORMAT_B5G5R5A1_UNORM);
	CHECK_FORMAT(FORMAT_B8G8R8A8_UNORM);
	CHECK_FORMAT(FORMAT_B8G8R8X8_UNORM);
	CHECK_FORMAT(FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
	CHECK_FORMAT(FORMAT_B8G8R8A8_TYPELESS);
	CHECK_FORMAT(FORMAT_B8G8R8A8_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_B8G8R8X8_TYPELESS);
	CHECK_FORMAT(FORMAT_B8G8R8X8_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_BC6H_TYPELESS);
	CHECK_FORMAT(FORMAT_BC6H_UF16);
	CHECK_FORMAT(FORMAT_BC6H_SF16);
	CHECK_FORMAT(FORMAT_BC7_TYPELESS);
	CHECK_FORMAT(FORMAT_BC7_UNORM);
	CHECK_FORMAT(FORMAT_BC7_UNORM_SRGB);
	CHECK_FORMAT(FORMAT_AYUV);
	CHECK_FORMAT(FORMAT_Y410);
	CHECK_FORMAT(FORMAT_Y416);
	CHECK_FORMAT(FORMAT_NV12);
	CHECK_FORMAT(FORMAT_P010);
	CHECK_FORMAT(FORMAT_P016);
	CHECK_FORMAT(FORMAT_420_OPAQUE);
	CHECK_FORMAT(FORMAT_YUY2);
	CHECK_FORMAT(FORMAT_Y210);
	CHECK_FORMAT(FORMAT_Y216);
	CHECK_FORMAT(FORMAT_NV11);
	CHECK_FORMAT(FORMAT_AI44);
	CHECK_FORMAT(FORMAT_IA44);
	CHECK_FORMAT(FORMAT_P8);
	CHECK_FORMAT(FORMAT_A8P8);
	CHECK_FORMAT(FORMAT_B4G4R4A4_UNORM);
	CHECK_FORMAT(FORMAT_P208);
	CHECK_FORMAT(FORMAT_V208);
	CHECK_FORMAT(FORMAT_V408);
	CHECK_FORMAT(FORMAT_FORCE_UINT);
	return (EFORMAT)0;

#undef CHECK_FORMAT
}

static EPOLYGON_TYPE ParsePolygonType(const std::string& PolygonStr)
{
#define CHECK_POLYGON_MODE(x) if (PolygonStr == #x) { return x; }

	CHECK_POLYGON_MODE(POLYGON_TYPE_POINTS);
	CHECK_POLYGON_MODE(POLYGON_TYPE_LINES);
	CHECK_POLYGON_MODE(POLYGON_TYPE_TRIANGLES);
	CHECK_POLYGON_MODE(POLYGON_TYPE_TRIANGLE_STRIPS);
	return POLYGON_TYPE_TRIANGLES;

#undef CHECK_POLYGON_MODE
}

static EBLEND_OP ParseBlendOp(const std::string& BlendOpStr)
{
#define CHECK_BLEND_OP(x) if (BlendOpStr == #x) { return x; }

	CHECK_BLEND_OP(BLEND_OP_ADD);
	CHECK_BLEND_OP(BLEND_OP_SUBTRACT);
	CHECK_BLEND_OP(BLEND_OP_REV_SUBTRACT);
	CHECK_BLEND_OP(BLEND_OP_MIN);
	CHECK_BLEND_OP(BLEND_OP_MAX);
	return (EBLEND_OP)0;

#undef CHECK_BLEND_OP
}

static ELOGIC_OP ParseLogicOp(const std::string& LogicOpStr)
{
#define CHECK_LOGIC_OP(x) if (LogicOpStr == #x) { return x; }

	CHECK_LOGIC_OP(LOGIC_OP_CLEAR);
	CHECK_LOGIC_OP(LOGIC_OP_SET);
	CHECK_LOGIC_OP(LOGIC_OP_COPY);
	CHECK_LOGIC_OP(LOGIC_OP_COPY_INVERTED);
	CHECK_LOGIC_OP(LOGIC_OP_NOOP);
	CHECK_LOGIC_OP(LOGIC_OP_INVERT);
	CHECK_LOGIC_OP(LOGIC_OP_AND);
	CHECK_LOGIC_OP(LOGIC_OP_NAND);
	CHECK_LOGIC_OP(LOGIC_OP_OR);
	CHECK_LOGIC_OP(LOGIC_OP_NOR);
	CHECK_LOGIC_OP(LOGIC_OP_XOR);
	CHECK_LOGIC_OP(LOGIC_OP_EQUIV);
	CHECK_LOGIC_OP(LOGIC_OP_AND_REVERSE);
	CHECK_LOGIC_OP(LOGIC_OP_AND_INVERTED);
	CHECK_LOGIC_OP(LOGIC_OP_OR_REVERSE);
	CHECK_LOGIC_OP(LOGIC_OP_OR_INVERTED);
	return (ELOGIC_OP)0;

#undef CHECK_LOGIC_OP
}

static ECOMPARISON_FUNCTION ParseComparisonFunction(const std::string& ComparisonFuncStr)
{
#define CHECK_COMPARISON_FUNC(x) if (ComparisonFuncStr == #x) { return x; }

	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_NEVER);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_LESS);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_LESS_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_GREATER);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_NOT_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_GREATER_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_ALWAYS);
	return (ECOMPARISON_FUNCTION)0;

#undef CHECK_COMPARISON_FUNC
}

// static ESTENCIL_OP ParseStencilOp(const std::string StencilOpStr)
// {
// #define CHECK_STENCIL_OP(x) if (StencilOpStr == #x) { return x; }
// 
// 	CHECK_STENCIL_OP(STENCIL_OP_KEEP);
// 	CHECK_STENCIL_OP(STENCIL_OP_ZERO);
// 	CHECK_STENCIL_OP(STENCIL_OP_REPLACE);
// 	CHECK_STENCIL_OP(STENCIL_OP_INCR_SAT);
// 	CHECK_STENCIL_OP(STENCIL_OP_DECR_SAT);
// 	CHECK_STENCIL_OP(STENCIL_OP_INVERT);
// 	CHECK_STENCIL_OP(STENCIL_OP_INCR);
// 	CHECK_STENCIL_OP(STENCIL_OP_DECR);
// 	return (ESTENCIL_OP)0;
// 
// #undef CHECK_STENCIL_OP
// }



// static ESTENCIL_OP ParseStencilOp(const std::string StencilOpStr)
// {
// #define CHECK_STENCIL_OP(x) if (StencilOpStr == #x) { return x; }
// 
// 	CHECK_STENCIL_OP(STENCIL_OP_KEEP);
// 	CHECK_STENCIL_OP(STENCIL_OP_ZERO);
// 	CHECK_STENCIL_OP(STENCIL_OP_REPLACE);
// 	CHECK_STENCIL_OP(STENCIL_OP_INCR_SAT);
// 	CHECK_STENCIL_OP(STENCIL_OP_DECR_SAT);
// 	CHECK_STENCIL_OP(STENCIL_OP_INVERT);
// 	CHECK_STENCIL_OP(STENCIL_OP_INCR);
// 	CHECK_STENCIL_OP(STENCIL_OP_DECR);
// 	return (ESTENCIL_OP)0;
// 
// #undef CHECK_STENCIL_OP
// }

static std::string ParseToEndOfScope(const std::string& InStr, size_t StartParse)
{
	std::string Result;
	size_t ScopeEndLoc = 0;
	uint32_t Depth = 1;
	size_t BeginScopeLoc = InStr.find('{', StartParse);
	size_t CurrentLoc = BeginScopeLoc;
	for (int i = 0; i < 10000; i++) // Protect from infinite loop condition
	{
		if (i > 9999)
		{
			__debugbreak(); // Something really bad happened here
		}
		size_t RightSideLoc = InStr.find('}', CurrentLoc);
		size_t LeftSideLoc = InStr.find('{', CurrentLoc);

		if (LeftSideLoc == BeginScopeLoc)
		{
			if (RightSideLoc > LeftSideLoc) // Ew redundant code
			{
				LeftSideLoc = InStr.find('{', LeftSideLoc + 1);
				if (RightSideLoc > LeftSideLoc)
				{
					CurrentLoc = RightSideLoc;
					Depth++;
				}
				else
				{
					CurrentLoc = RightSideLoc + 1;
					Depth--;
				}
			}
			else
			{
				Depth--; //???
			}
		}
		else if (RightSideLoc > LeftSideLoc) // If we have a new opening brace, ie if we found another opening brace before we found a closing brace
		{
			CurrentLoc = RightSideLoc;
			Depth++;
		}
		else if (RightSideLoc < LeftSideLoc) // if we have a new closing brace
		{
			CurrentLoc = RightSideLoc + 1;
			Depth--;
		}

		if (Depth == 0)
		{
			ScopeEndLoc = CurrentLoc;
			break;
		}
	}
	Result = InStr.substr(BeginScopeLoc + 1, ((ScopeEndLoc - StartParse) - 3));
	return Result;
}

static void ParseRenderTargetDesc(const std::string& DescStr, GFX_RENDER_TARGET_DESC& OutDesc)
{
	OutDesc = CreateDefaultGFXRenderTargetDesc();
	{
		size_t BlendEnableLoc = DescStr.find("BlendEnable");
		if (BlendEnableLoc != std::string::npos)
		{
			std::string BlendEnableStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", BlendEnableLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					BlendEnableStr.push_back(DescStr[i]);
				}
			}
			OutDesc.bBlendEnable = ParseBool(BlendEnableStr);
		}
	}
	{
		size_t LogicEnableLoc = DescStr.find("LogicEnable");
		if (LogicEnableLoc != std::string::npos)
		{
			std::string LogicEnableStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", LogicEnableLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					LogicEnableStr.push_back(DescStr[i]);
				}
			}
			OutDesc.bLogicOpEnable = ParseBool(LogicEnableStr);
		}
	}
	{
		size_t SrcBlendLoc = DescStr.find("SrcBlend");
		if (SrcBlendLoc != std::string::npos)
		{
			std::string SrcBlendStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", SrcBlendLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					SrcBlendStr.push_back(DescStr[i]);
				}
			}
			OutDesc.SrcBlend = ParseBlendStyle(SrcBlendStr);
		}
	}
	{
		size_t DstBlendLoc = DescStr.find("DstBlend");
		if (DstBlendLoc != std::string::npos)
		{
			std::string DstBlendStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", DstBlendLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					DstBlendStr.push_back(DescStr[i]);
				}
			}
			OutDesc.DstBlend = ParseBlendStyle(DstBlendStr);
		}
	}
	{
		size_t BlendOpLoc = DescStr.find("BlendOp");
		if (BlendOpLoc != std::string::npos)
		{
			std::string BlendOpStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", BlendOpLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					BlendOpStr.push_back(DescStr[i]);
				}
			}
			OutDesc.BlendOp = ParseBlendOp(BlendOpStr);
		}
	}
	{
		size_t SrcBlendAlphaLoc = DescStr.find("SrcBlendAlpha");
		if (SrcBlendAlphaLoc != std::string::npos)
		{
			std::string SrcBlendAlphaStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", SrcBlendAlphaLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					SrcBlendAlphaStr.push_back(DescStr[i]);
				}
			}
			OutDesc.SrcBlendAlpha = ParseBlendStyle(SrcBlendAlphaStr);
		}
	}
	{
		size_t DstBlendAlphaLoc = DescStr.find("DstBlendAlpha");
		if (DstBlendAlphaLoc != std::string::npos)
		{
			std::string DstBlendAlphaStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", DstBlendAlphaLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					DstBlendAlphaStr.push_back(DescStr[i]);
				}
			}
			OutDesc.DstBlendAlpha = ParseBlendStyle(DstBlendAlphaStr);
		}
	}
	{
		size_t AlphaBlendOpLoc = DescStr.find("AlphaBlendOp");
		if (AlphaBlendOpLoc != std::string::npos)
		{
			std::string AlphaBlendOpStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", AlphaBlendOpLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					AlphaBlendOpStr.push_back(DescStr[i]);
				}
			}
			OutDesc.AlphaBlendOp = ParseBlendOp(AlphaBlendOpStr);
		}
	}
	{
		size_t LogicOpLoc = DescStr.find("LogicOp");
		if (LogicOpLoc != std::string::npos)
		{
			std::string LogicOpStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", LogicOpLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					LogicOpStr.push_back(DescStr[i]);
				}
			}
			OutDesc.LogicOp = ParseLogicOp(LogicOpStr);
		}
	}
	{
		size_t FormatLoc = DescStr.find("Format");
		if (FormatLoc != std::string::npos)
		{
			std::string FormatStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", FormatLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					FormatStr.push_back(DescStr[i]);
				}
			}
			OutDesc.Format = ParseFormat(FormatStr);
		}
	}
}

/*
static void ParseStencilOpDesc(const std::string& DescStr, GFX_DEPTH_STENCIL_OP_DESC& OutDesc)
{
OutDesc = { STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP, COMPARISON_FUNCTION_LESS };

{
size_t StencilFailOpLoc = DescStr.find("StencilFailOp");
if (StencilFailOpLoc != std::string::npos)
{
std::string StencilFailOpStr;
size_t AssignmentOperatorLoc = DescStr.find("=", StencilFailOpLoc);
size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
{
if (DescStr[i] != ' ' && DescStr[i] != '\n')
{
StencilFailOpStr.push_back(DescStr[i]);
}
}
OutDesc.StencilFailOp = ParseStencilOp(StencilFailOpStr);
}
}
{
size_t StencilDepthFailOpLoc = DescStr.find("StencilDepthFailOp");
if (StencilDepthFailOpLoc != std::string::npos)
{
std::string StencilDepthFailOpStr;
size_t AssignmentOperatorLoc = DescStr.find("=", StencilDepthFailOpLoc);
size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
{
if (DescStr[i] != ' ' && DescStr[i] != '\n')
{
StencilDepthFailOpStr.push_back(DescStr[i]);
}
}
OutDesc.StencilDepthFailOp = ParseStencilOp(StencilDepthFailOpStr);
}
}
{
size_t StencilPassOpLoc = DescStr.find("StencilPassOp");
if (StencilPassOpLoc != std::string::npos)
{
std::string StencilPassOpStr;
size_t AssignmentOperatorLoc = DescStr.find("=", StencilPassOpLoc);
size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
{
if (DescStr[i] != ' ' && DescStr[i] != '\n')
{
StencilPassOpStr.push_back(DescStr[i]);
}
}
OutDesc.StencilPassOp = ParseStencilOp(StencilPassOpStr);
}
}
{
size_t ComparisonFuncOp = DescStr.find("ComparisonFunction");
if (ComparisonFuncOp != std::string::npos)
{
std::string ComparisonFuncStr;
size_t AssignmentOperatorLoc = DescStr.find("=", ComparisonFuncOp);
size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
{
if (DescStr[i] != ' ' && DescStr[i] != '\n')
{
ComparisonFuncStr.push_back(DescStr[i]);
}
}
OutDesc.ComparisonFunction = ParseComparisonFunction(ComparisonFuncStr);
}
}
}


static void FillDescriptor(const std::string& InDescStr, FULL_PIPELINE_DESCRIPTOR& OutDesc, bool& bUsesCustomDepthStencilState)
{
	uint32_t NumRenderTargets = 0;

	std::string DescStr = InDescStr;
	RemoveTabs(DescStr);
	bUsesCustomDepthStencilState = false;
	{
		std::string SearchStr = "RenderTarget__";
		for (size_t i = 0; i < 8; i++)
		{
			SearchStr[13] = std::to_string(i)[0];
			size_t DeclPos = DescStr.find(SearchStr, 0);

			if (DeclPos != std::string::npos)
			{
				size_t BeginScopePos = DescStr.find("{", DeclPos);
				size_t EndScopePos = DescStr.find("}", DeclPos);
				std::string RenderTargetDesc = DescStr.substr(BeginScopePos, EndScopePos - BeginScopePos);
				ParseRenderTargetDesc(RenderTargetDesc, OutDesc.RtvDescs[i]);
			}
		}
		OutDesc.NumRenderTargets = NumRenderTargets;
	}
	{

		bool bDepthWrite = true;
		size_t UseDepthStencilLoc = DescStr.find("DepthWrite", 0);

		if (UseDepthStencilLoc != std::string::npos)
		{
			size_t EndOfDepthStenilLoc = DescStr.find(";", UseDepthStencilLoc);
			std::string UseDepthStencilStr;
			for (size_t i = UseDepthStencilLoc; i < EndOfDepthStenilLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					UseDepthStencilStr.push_back(DescStr[i]);
				}
			}
			bUsesCustomDepthStencilState = true;
			bDepthWrite = ParseBool(UseDepthStencilStr);
		}

		if (bDepthWrite)
		{
			OutDesc.DepthStencilState = GEnableDepthWriteDesc;
		}
		else
		{
			OutDesc.DepthStencilState = GDisableDepthWriteDesc;
		}
	}
	size_t DepthStencilDescLoc = DescStr.find("DepthStencilState");

	if (DepthStencilDescLoc != std::string::npos)
	{
		size_t BeginDeclLoc = DescStr.find('{', DepthStencilDescLoc);

		std::string DepthStencilDesc = ParseToEndOfScope(DescStr, BeginDeclLoc);

		GFX_DEPTH_STENCIL_DESC Desc = CreateDefaultGFXDepthStencilDesc();

		{
			size_t FormatLoc = DepthStencilDesc.find("Format");
			if (FormatLoc != std::string::npos)
			{
				std::string FormatStr;
				size_t AssignmentOperatorLoc = DepthStencilDesc.find("=", FormatLoc);
				size_t EndAssignmentLoc = DepthStencilDesc.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (DepthStencilDesc[i] != ' ' && DepthStencilDesc[i] != '\n')
					{
						FormatStr.push_back(DepthStencilDesc[i]);
					}
				}
				Desc.Format = ParseFormat(FormatStr);
			}
		}
		{
			size_t DepthEnableLoc = DepthStencilDesc.find("DepthEnable");
			if (DepthEnableLoc != std::string::npos)
			{
				std::string DepthEnableStr;
				size_t AssignmentOperatorLoc = DepthStencilDesc.find("=", DepthEnableLoc);
				size_t EndAssignmentLoc = DepthStencilDesc.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (DepthStencilDesc[i] != ' ' && DepthStencilDesc[i] != '\n')
					{
						DepthEnableStr.push_back(DepthStencilDesc[i]);
					}
				}
				bUsesCustomDepthStencilState = true;
				Desc.bDepthEnable = ParseBool(DepthEnableStr);
			}
		}
		{
			size_t DepthFuncitionLoc = DepthStencilDesc.find("DepthFunction");
			if (DepthFuncitionLoc != std::string::npos)
			{
				std::string DepthFunctionStr;
				size_t AssignmentOperatorLoc = DepthStencilDesc.find("=", DepthFuncitionLoc);
				size_t EndAssignmentLoc = DepthStencilDesc.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (DepthStencilDesc[i] != ' ' && DepthStencilDesc[i] != '\n')
					{
						DepthFunctionStr.push_back(DepthStencilDesc[i]);
					}
				}
				bUsesCustomDepthStencilState = true;
				Desc.DepthFunction = ParseComparisonFunction(DepthFunctionStr);
			}
		}
		{
			size_t DepthEnableLoc = DepthStencilDesc.find("StencilEnable");
			if (DepthEnableLoc != std::string::npos)
			{
				std::string StencilEnableStr;
				size_t AssignmentOperatorLoc = DepthStencilDesc.find("=", DepthEnableLoc);
				size_t EndAssignmentLoc = DepthStencilDesc.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (DepthStencilDesc[i] != ' ' && DepthStencilDesc[i] != '\n')
					{
						StencilEnableStr.push_back(DepthStencilDesc[i]);
					}
				}
				bUsesCustomDepthStencilState = true;
				Desc.bStencilEnable = ParseBool(StencilEnableStr);
			}
		}
		OutDesc.DepthStencilState = Desc;
	}

	size_t RasterizerStateLoc = DescStr.find("RasterState");

	if (RasterizerStateLoc != std::string::npos)
	{
		GFX_RASTER_DESC RasterDesc = CreateDefaultGFXRasterDesc();
		size_t BeginDeclLoc = DescStr.find('{', RasterizerStateLoc);

		std::string RasterDescStr = ParseToEndOfScope(DescStr, BeginDeclLoc);

		{
			size_t FillSolidLoc = RasterDescStr.find("FillSolid");
			if (FillSolidLoc != std::string::npos)
			{
				std::string FillSolidStr;
				size_t AssignmentOperatorLoc = RasterDescStr.find("=", FillSolidLoc);
				size_t EndAssignmentLoc = RasterDescStr.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (RasterDescStr[i] != ' ' && RasterDescStr[i] != '\n')
					{
						FillSolidStr.push_back(RasterDescStr[i]);
					}
				}
				RasterDesc.bFillSolid = ParseBool(FillSolidStr);
			}
		}
		{
			size_t CullEnableLoc = RasterDescStr.find("Cull");
			if (CullEnableLoc != std::string::npos)
			{
				std::string CullEnableStr;
				size_t AssignmentOperatorLoc = RasterDescStr.find("=", CullEnableLoc);
				size_t EndAssignmentLoc = RasterDescStr.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (RasterDescStr[i] != ' ' && RasterDescStr[i] != '\n')
					{
						CullEnableStr.push_back(RasterDescStr[i]);
					}
				}
				RasterDesc.bCull = ParseBool(CullEnableStr);
			}
		}
		{
			size_t CounterClockwiseForwardLoc = RasterDescStr.find("IsCounterClockwiseForward");
			if (CounterClockwiseForwardLoc != std::string::npos)
			{
				std::string CounterClockwiseForwardStr;
				size_t AssignmentOperatorLoc = RasterDescStr.find("=", CounterClockwiseForwardLoc);
				size_t EndAssignmentLoc = RasterDescStr.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (RasterDescStr[i] != ' ' && RasterDescStr[i] != '\n')
					{
						CounterClockwiseForwardStr.push_back(RasterDescStr[i]);
					}
				}
				RasterDesc.bIsCounterClockwiseForward = ParseBool(CounterClockwiseForwardStr);
			}
		}
		{
			size_t DepthClipEnableLoc = RasterDescStr.find("DepthClipEnable");
			if (DepthClipEnableLoc != std::string::npos)
			{
				std::string DepthClipEnableStr;
				size_t AssignmentOperatorLoc = RasterDescStr.find("=", DepthClipEnableLoc);
				size_t EndAssignmentLoc = RasterDescStr.find(";", AssignmentOperatorLoc);
				for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
				{
					if (RasterDescStr[i] != ' ' && RasterDescStr[i] != '\n')
					{
						DepthClipEnableStr.push_back(RasterDescStr[i]);
					}
				}
				RasterDesc.bDepthClipEnable = ParseBool(DepthClipEnableStr);
			}
		}

		OutDesc.RasterDesc = RasterDesc;
	}

	{
		size_t TopologyTypeLoc = DescStr.find("PolygonType");
		if (TopologyTypeLoc != std::string::npos)
		{
			std::string TopologyStr;
			size_t AssignmentOperatorLoc = DescStr.find("=", TopologyTypeLoc);
			size_t EndAssignmentLoc = DescStr.find(";", AssignmentOperatorLoc);
			for (size_t i = AssignmentOperatorLoc + 1; i < EndAssignmentLoc; i++)
			{
				if (DescStr[i] != ' ' && DescStr[i] != '\n')
				{
					TopologyStr.push_back(DescStr[i]);
				}
			}
			OutDesc.PolygonType = ParsePolygonType(TopologyStr);
		}
	}
}

static bool IsSystemType(const VARIABLE_INFO& InVar)
{
	if (InVar.Type == "float" || InVar.Type == "float2" || InVar.Type == "float3" || InVar.Type == "float4" ||
		InVar.Type == "int" || InVar.Type == "int2" || InVar.Type == "int3" || InVar.Type == "int4" ||
		InVar.Type == "uint32" || InVar.Type == "uint322" || InVar.Type == "uint323" || InVar.Type == "uint324" ||
		InVar.Type == "float2x2" || InVar.Type == "float2x3" || InVar.Type == "float2x4" ||
		InVar.Type == "float3x2" || InVar.Type == "float3x3" || InVar.Type == "float3x4" ||
		InVar.Type == "float4x2" || InVar.Type == "float4x3" || InVar.Type == "float4x4" ||
		InVar.Type == "bool")
	{
		return true;
	}
	return false;
}

static std::string FindStructDeclaration(const std::string& InFile, const std::string& StructureName)
{
	std::string File = InFile;
	std::string Result;
	size_t CurrentLoc = 0;

	bool bFound = false;

	for (size_t i = 0; i < 800; i++)
	{
		if (i == 799)
		{
			__debugbreak();
		}

		CurrentLoc = InFile.find("struct", CurrentLoc);

		if (CurrentLoc != std::string::npos)
		{
			for (; InFile[CurrentLoc] == ' ' || InFile[CurrentLoc] == '\n'; CurrentLoc++) {}

			size_t BeginDefinitionLoc = InFile.find('{', CurrentLoc);

			std::string CheckName;
			for (i = CurrentLoc + 6; i != BeginDefinitionLoc; i++)
			{
				char C = InFile[i];
				if (C != ' ' && C != '\n')
				{
					CheckName.push_back(C);
				}
			}
			if (CheckName == StructureName)
			{
				bFound = true;
				break;
			}
			else
			{
				CurrentLoc++;
			}
		}
		else
		{
			__debugbreak();
		}
	}
	if (bFound)
	{
		for (; InFile[CurrentLoc] != '{'; CurrentLoc++)
		{
			char C = InFile[CurrentLoc];
			Result.push_back(C);
		}
		CurrentLoc--;
	}

	Result += "{\n" + ParseToEndOfScope(InFile, CurrentLoc) + "\n};";
	return Result;
}

static std::string FindFunctionDeclaration(const std::string& File, const std::string& FunctionName)
{
	std::string Result;
	size_t FuncNameDecl = File.find(FunctionName);
	size_t CurrentPos = FuncNameDecl;

	// Here we try to find the return type

	if (FuncNameDecl == std::string::npos)
	{
		__debugbreak();
		return "";
	}

	for (; File[CurrentPos] != ' ' && File[CurrentPos] != '\n'; CurrentPos--) {}
	CurrentPos--;
	for (; File[CurrentPos] != ' ' && File[CurrentPos] != '\n'; CurrentPos--) {}
	CurrentPos--;

	size_t EndFuncDecl = File.find(')', FuncNameDecl);

	for (; CurrentPos < EndFuncDecl + 2; CurrentPos++)
	{
		if (File[CurrentPos] != '{')
		{
			Result.push_back(File[CurrentPos]);
		}
		else
		{
			break;
		}
	}
	return Result;
}



void Init()
{
	GEnableDepthWriteDesc = CreateDefaultGFXDepthStencilDesc();
	GDisableDepthWriteDesc = GEnableDepthWriteDesc;
	GDisableDepthWriteDesc.bDepthEnable = false;
}

void GetResourceCounts(const std::string& InFile, PIPELINE_RESOURCE_COUNTERS& Counts)
{
	uint8_t& OutNumConstantBuffers = Counts.NumConstantBuffers;
	uint8_t& OutNumSamplers = Counts.NumSamplers;
	uint8_t& OutNumShaderResourceViews = Counts.NumShaderResourceViews;
	uint8_t& OutNumUnorderedAccessViews = Counts.NumUnorderedAccessViews;
	std::string ByteCodeAsString = InFile;
	{
		uint8_t NumConstBuffers = 0;

		for (size_t i = 0; i < ByteCodeAsString.length(); i++)
		{
			i = ByteCodeAsString.find("register(b", i);
			if (i == std::string::npos)
				break;
			NumConstBuffers++;
		}
		OutNumConstantBuffers = NumConstBuffers;
	}
	{
		uint8_t NumSamplers = 0;
		for (size_t i = 0; i < ByteCodeAsString.length(); i++)
		{
			i = ByteCodeAsString.find("register(s", i);
			if (i == std::string::npos)
				break;
			NumSamplers++;
		}
		OutNumSamplers = NumSamplers;
	}
	{
		uint8_t NumShaderResources = 0;
		for (size_t i = 0; i < ByteCodeAsString.length(); i++)
		{
			i = ByteCodeAsString.find("register(t", i);
			if (i == std::string::npos)
				break;
			NumShaderResources++;
		}
		OutNumShaderResourceViews = NumShaderResources;
	}
	{
		uint8_t NumUnorderedAccessViews = 0;
		for (size_t i = 0; i < ByteCodeAsString.length(); i++)
		{
			i = ByteCodeAsString.find("register(u", i);
			if (i == std::string::npos)
				break;
			NumUnorderedAccessViews++;
		}
		OutNumUnorderedAccessViews = NumUnorderedAccessViews;
	}
}

FULL_PIPELINE_DESCRIPTOR ParseGraphicsShader(const std::string& InFile, const std::string& FileName)
{
	std::string File = InFile;
	RemoveComments(File);

	FULL_PIPELINE_DESCRIPTOR PipelineDesc = CreateDefaultDescriptor();

	auto Find = File.find("CAMERA_CONSTANTS");
	if (Find != std::string::npos)
	{
		File.replace(Find, 16, 1, ' ');
		std::string CameraConstantsStr = "cbuffer CameraConstants : register(b0)\n{\n\tmatrix Model;\n\tmatrix View;\n\tmatrix Projection;\n\tfloat4 Position;\n}";
		File.insert(Find, CameraConstantsStr.c_str(), 109);
	}

	std::string Resources;
	std::string PipelineDescStr;
	size_t ResourcesStartLoc;
	bool bUsesCustomDepthStencil = false;

	GetResourceCounts(InFile, PipelineDesc.Counts);

	// Find Resources
	{
		ResourcesStartLoc = File.find("Resources", 0);
		if (ResourcesStartLoc != std::string::npos)
		{
			File.erase(ResourcesStartLoc, 9);
			Resources = ParseToEndOfScope(File, ResourcesStartLoc);
			size_t Range = Resources.length() + 4;
			RemoveTabs(Resources);
			CutStringRange(File, ResourcesStartLoc, Range);
		}
	}
	{
		size_t PipelineLoc = File.find("Pipeline");
		if (PipelineLoc == std::string::npos)
		{
			// Here there is a very real possibility that they want to use the default pipeline, therefore just make a default one but find the input layout
		}
		else
		{
			File.erase(PipelineLoc, 8);

			PipelineDescStr = ParseToEndOfScope(File, PipelineLoc);
			size_t Range = PipelineDescStr.length() + 4;
			CutStringRange(File, PipelineLoc + 1, Range);
			RemoveTabs(PipelineDescStr);
			FillDescriptor(PipelineDescStr, PipelineDesc, bUsesCustomDepthStencil);
		}
	}
	{
		FUNCTION_INFO VSMainInfo = GetFunctionInfo(FindFunctionDeclaration(File, "VSMain"));

		std::vector<VARIABLE_INFO> VSInputs;
		for (uint32_t i = 0; i < VSMainInfo.Parameters.size(); i++)
		{
			VARIABLE_INFO& VarInfo = VSMainInfo.Parameters[i];

			// If its a system type then we assume that the semantics are passed in here
			if (IsSystemType(VarInfo))
			{
				if (VarHasSemantic(VarInfo) && VarInfo.Semantic.find("SV_") == std::string::npos)
				{
					VSInputs.push_back(VarInfo);
				}
			}
			else
			{
				std::string StructName = VarInfo.Type;
				std::string StructDeclaration = FindStructDeclaration(File, StructName);
				STRUCT_INFO StructureInfo = GetStructInfo(StructDeclaration);

				for (auto& Var : StructureInfo.Variables)
				{
					if (VarHasSemantic(Var) && Var.Semantic.find("SV_") == std::string::npos)
					{
						VSInputs.push_back(Var);
					}
				}
			}
		}
		{
			GFX_INPUT_LAYOUT_DESC InputLayoutDesc;
			for (auto& Parameter : VSInputs)
			{
				GFX_INPUT_ITEM_DESC InputItem;
				InputItem.Name = Parameter.Semantic;
				InputItem.ItemFormat = TypeToItemFormat(Parameter.Type);
				InputLayoutDesc.InputItems.push_back(InputItem);
			}
			PipelineDesc.InputLayout = InputLayoutDesc;
		}
	}

	size_t NumRenderTargets = 0;

	{
		std::string SearchStr = "SV_TARGET_";
		for (size_t i = 0; i < 8; i++)
		{
			SearchStr[9] = std::to_string(i)[0];
			size_t RTFoundLoc = File.find(SearchStr, 0);

			if (RTFoundLoc != std::string::npos)
			{
				NumRenderTargets++;
			}
			else
			{
				break;
			}
		}
	}

	{
		std::string FullShader = File;
		if (ResourcesStartLoc != std::string::npos)
		{
			FullShader.insert(ResourcesStartLoc, Resources.c_str());
		}
		bool bHasHullShader = (File.find("HSMain", 0) != std::string::npos);
		bool bHasGeometryShader = (File.find("GSMain", 0) != std::string::npos);
		bool bHasDomainShader = (File.find("DSMain", 0) != std::string::npos);

		PipelineDesc.VS = CompileVertexShader(FullShader, _placeholder_);
		PipelineDesc.PS = CompilePixelShader(FullShader, _placeholder_);

		if (PipelineDesc.VS.CompiledStages[0].ByteCode.size() == 0)
		{
			std::cout << "[ERROR] " << FileName << " Vertex Shader Failed to compile" << std::endl;
			std::cin.get();
			return ParseGraphicsShader(ReadEntireFile(FileName), FileName);
		}
		if (PipelineDesc.PS.CompiledStages[0].ByteCode.size() == 0)
		{
			std::cout << "[ERROR] " << FileName << " Pixel Shader Failed to compile" << std::endl;
			std::cin.get();
			return ParseGraphicsShader(ReadEntireFile(FileName), FileName);
		}

		if (bHasHullShader)
		{
			PipelineDesc.HS = CompileHullShader(FullShader, _placeholder_);
			if (PipelineDesc.HS.CompiledStages[0].ByteCode.size() == 0)
			{
				std::cout << "[ERROR] " << FileName << " Hull Shader Failed to compile" << std::endl;
				std::cin.get();
				return ParseGraphicsShader(ReadEntireFile(FileName), FileName);
			}
		}
		if (bHasDomainShader)
		{
			PipelineDesc.DS = CompileDomainShader(FullShader, _placeholder_);
			if (PipelineDesc.DS.CompiledStages[0].ByteCode.size() == 0)
			{
				std::cout << "[ERROR] " << FileName << " Domain Shader Failed to compile" << std::endl;
				std::cin.get();
				return ParseGraphicsShader(ReadEntireFile(FileName), FileName);
			}
		}
		if (bHasGeometryShader)
		{
			PipelineDesc.GS = CompileGeometryShader(FullShader, _placeholder_);
			if (PipelineDesc.GS.CompiledStages[0].ByteCode.size() == 0)
			{
				std::cout << "[ERROR] " << FileName << " Geometry Shader Failed to compile" << std::endl;
				std::cin.get();
				return ParseGraphicsShader(ReadEntireFile(FileName), FileName);
			}
		}

		if (bUsesCustomDepthStencil == false)
		{
			PipelineDesc.DepthStencilState = GEnableDepthWriteDesc;
		}
	}

	PipelineDesc.NumRenderTargets = static_cast<uint32_t>(NumRenderTargets);

	std::cout << "[MESSAGE] Compiled " << FileName << std::endl;

	return PipelineDesc;
}

COMPUTE_PIPELINE_DESC ParseComputeShader(const std::string& InFile, const std::string& FileName)
{
	COMPUTE_PIPELINE_DESC Result = {};

	GetResourceCounts(InFile, Result.Counts);

	Result.CS = CompileComputeShader(InFile, _placeholder_);

	if (Result.CS.CompiledStages[0].ByteCode.size() == 0)
	{
		std::cout << "[ERROR] Failed to compile compute shader: " << FileName;
		std::cin.get();
		return ParseComputeShader(ReadEntireFile(FileName), FileName);
	}
	std::cout << "[MESSAGE] Compiled " << FileName << std::endl;

	return Result;
}

RAYTRACING_PIPELINE_DESC ParseRaytracingShader(const std::string& InFile, const std::string& FileName)
{
	RAYTRACING_PIPELINE_DESC Result = {};

	GetResourceCounts(InFile, Result.Counts);

	Result.Library = CompileRaytracingShader(InFile, _placeholder_);

	// RayIntersection
	// RayAnyHit
	// RayClosestHit

	std::string NoComments = InFile;
	RemoveComments(NoComments);

	// Now we're safe to search for function names
	// This is a naive approach however its *probably* good enough

	// We're also assuming that "RayGen" and "RayMiss" are present already

	if (NoComments.find("RayIntersection") != std::string::npos)
	{
		Result.bHasIntersection = true;
	}

	if (NoComments.find("RayAnyHit") != std::string::npos)
	{
		Result.bHasAnyHit = true;
	}

	if (NoComments.find("RayClosestHit") != std::string::npos)
	{
		Result.bHasClosestHit = true;
	}

	if (Result.Library.CompiledStages[0].ByteCode.size() == 0)
	{
		std::cout << "[ERROR] Failed to compile raytracing shader: " << FileName;
		std::cin.get();
		return ParseRaytracingShader(ReadEntireFile(FileName), FileName);
	}

	std::cout << "[MESSAGE] Compiled " << FileName << std::endl;

	return Result;
}

FULL_PIPELINE_DESCRIPTOR LoadGraphicsPipeline(const std::string& ShaderFile)
{
	std::string File = ReadEntireFile(ShaderFile);
	FULL_PIPELINE_DESCRIPTOR PipelineDesc = ParseGraphicsShader(File, ShaderFile);
	return PipelineDesc;
}

COMPUTE_PIPELINE_DESC LoadComputePipeline(const std::string& ShaderFile)
{
	std::string File = ReadEntireFile(ShaderFile);
	COMPUTE_PIPELINE_DESC PipelineDesc = ParseComputeShader(File, ShaderFile);
	return PipelineDesc;
}

RAYTRACING_PIPELINE_DESC LoadRaytracingPipeline(const std::string& ShaderFile)
{
	std::string File = ReadEntireFile(ShaderFile);
	RAYTRACING_PIPELINE_DESC PipelineDesc = ParseRaytracingShader(File, ShaderFile);
	return PipelineDesc;
}
*/

PipelineCompiler::PipelineCompiler(ShaderCompiler* compiler) :
	m_Compiler(compiler)
{
}

void PipelineCompiler::SetSrcDir(const std::filesystem::path& path)
{
	m_SrcPath = path;
}

bool PipelineCompiler::Load()
{
	for (const auto& file : std::filesystem::directory_iterator(m_SrcPath))
	{
		// If its a directory or not file, skip
		if (!std::filesystem::is_regular_file(file.status()))
		{
			continue;
		}

		std::string filename = file.path().filename().string();

		std::string ext = GetFileSuffix(filename);

		std::cout << "[INFO] Loading shader: " << filename << std::endl;
		if (ext == "ray")
		{
			if (!LoadRayFile(file.path()))
			{
				std::cout << "[ERROR] Failed to compile shader " << filename << std::endl;
			}
		}
		else if (ext == "gfx")
		{
			if (!LoadGfxFile(file.path()))
			{
				std::cout << "[ERROR] Failed to compile shader " << filename << std::endl;
			}
		}
		else if (ext == "cmpt")
		{
			if (!LoadCmptFile(file.path()))
			{
				std::cout << "[ERROR] Failed to compile shader " << filename << std::endl;
			}
		}
	}

	return true;
}

void PipelineCompiler::WriteToFile(const std::filesystem::path& dstFile)
{
	std::ofstream outFile(dstFile.native());
	outFile << m_Json;
}

bool PipelineCompiler::LoadGfxFile(const std::filesystem::path& path)
{
	FULL_PIPELINE_DESCRIPTOR desc = { };
	GraphicsAST ast(desc);

	if (!LoadFileImpl(path, &ast, "graphics"))
	{
		return false;
	}

	// I know its bad we're opening the same file
	// multiple times... TODO: fix?
	std::string fullFile = ReadEntireFile(path.filename().string());

	std::string toShader = CutPipelineBlock(fullFile, &ast);


	return true;
}

bool PipelineCompiler::LoadCmptFile(const std::filesystem::path& path)
{
	COMPUTE_PIPELINE_DESC desc = { };
	ComputeAST ast(desc);

	if (!LoadFileImpl(path, &ast, "compute"))
	{
		return false;
	}

	// I know its bad we're opening the same file
	// multiple times... TODO: fix?
	std::string fullFile = ReadEntireFile(path.filename().string());

	std::string toShader = CutPipelineBlock(fullFile, &ast);

	if (!m_Compiler->CompileComputeShader(toShader, &desc.CS))
	{
	}
	return true;
}

bool PipelineCompiler::LoadRayFile(const std::filesystem::path& path)
{
	RAYTRACING_PIPELINE_DESC desc = { };
	RaytracingAST ast(desc);

	if (!LoadFileImpl(path, &ast, "raytracing"))
	{
		return false;
	}

	// I know its bad we're opening the same file
	// multiple times... TODO: fix?
	std::string fullFile = ReadEntireFile(path.filename().string());

	std::string toShader = CutPipelineBlock(fullFile, &ast);

	if (!m_Compiler->CompileRaytracingShader(toShader, &desc.Library))
	{

	}

	return true;
}

bool PipelineCompiler::LoadFileImpl(const std::filesystem::path& path, ASTBase* ast, const std::string& type)
{
	if (!ast->LoadFile(path))
	{
		std::cout << "[ERROR] Failed to parse " << type << " shader" << path.filename().string() << std::endl;
		return false;
	}

	if (!ast->Interpret())
	{
		std::cout << "[ERROR] Failed to load " << type << " shader " << path.filename().string() << std::endl;
	}

	return true;
}

std::string PipelineCompiler::CutPipelineBlock(std::string& fileData, ASTBase* ast)
{
	uint32_t i = 0;
	uint32_t start = 0;
	uint32_t end = 0;
	std::stringstream str(fileData);
	std::stringstream out;
	std::string buf;
	ast->GetPipelineBlockLines(start, end);

	if (start == 0 && end == 0)
	{
		return fileData;
	}

	while (std::getline(str, buf))
	{
		if (i > start && i < end)
		{
			i++;
			continue;
		}
		out << buf << "\n";
		i++;
	}
	
	return out.str();
}
