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




using namespace nlohmann;
json RootObject;

static std::string ItemFormatToString(EINPUT_ITEM_FORMAT InFormat)
{
#define CHECK_INPUT_FORMAT(x) if (InFormat == x) { return #x; }
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_FLOAT);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_INT);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_FLOAT2);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_INT2);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_FLOAT3);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_INT3);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_FLOAT4);
	CHECK_INPUT_FORMAT(INPUT_ITEM_FORMAT_INT4);
	return "";
#undef CHECK_INPUT_FORMAT
}

// static std::string BoolToString(bool Bool)
// {
// 	if (Bool == true)
// 		return "true";
// 	if (Bool == false)
// 		return "false";
// 	return "";
// }

static std::string BlendStyleToString(const EBLEND_STYLE BlendStyle)
{
#define CHECK_BLEND_STYLE(x)  if (BlendStyle == x) { return #x; }

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
	return "";

#undef CHECK_BLEND_STYLE
}

inline std::string FormatToString(const EFORMAT Format)
{
#define CHECK_FORMAT(x) if (Format == x) { return #x; }

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
	return "";

#undef CHECK_FORMAT
}

static std::string BlendOpToString(const EBLEND_OP BlendOp)
{
#define CHECK_BLEND_OP(x) if (BlendOp == x) { return #x; }

	CHECK_BLEND_OP(BLEND_OP_ADD);
	CHECK_BLEND_OP(BLEND_OP_SUBTRACT);
	CHECK_BLEND_OP(BLEND_OP_REV_SUBTRACT);
	CHECK_BLEND_OP(BLEND_OP_MIN);
	CHECK_BLEND_OP(BLEND_OP_MAX);
	return "";

#undef CHECK_BLEND_OP
}

static std::string LogicOpToString(const ELOGIC_OP LogicOp)
{
#define CHECK_LOGIC_OP(x) if (LogicOp == x) { return #x; }

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
	return "";

#undef CHECK_LOGIC_OP
}

static std::string ComparisonFunctionToString(ECOMPARISON_FUNCTION ComparisonFunc)
{
#define CHECK_COMPARISON_FUNC(x) if (ComparisonFunc == x) { return #x; }

	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_NEVER);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_LESS);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_LESS_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_GREATER);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_NOT_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_GREATER_EQUAL);
	CHECK_COMPARISON_FUNC(COMPARISON_FUNCTION_ALWAYS);
	return "";

#undef CHECK_COMPARISON_FUNC
}

static std::string MultiSamplingLevelToString(EMULTISAMPLE_LEVEL MultiSampleLevel)
{
#define CHECK_MULTISAMPLING_LEVEL(x) if (MultiSampleLevel == x) { return #x; }

	CHECK_MULTISAMPLING_LEVEL(MULTISAMPLE_LEVEL_0);
	CHECK_MULTISAMPLING_LEVEL(MULTISAMPLE_LEVEL_4X);
	CHECK_MULTISAMPLING_LEVEL(MULTISAMPLE_LEVEL_8X);
	CHECK_MULTISAMPLING_LEVEL(MULTISAMPLE_LEVEL_16X);
	return "MULTISAMPLE_LEVEL_0";

#undef CHECK_MULTISAMPLING_LEVEL
}

static std::string PolygonModeToString(EPOLYGON_TYPE PolygonMode)
{
#define CHECK_POLYGON_MODE(x) if (PolygonMode == x) { return #x; }

	CHECK_POLYGON_MODE(POLYGON_TYPE_POINTS);
	CHECK_POLYGON_MODE(POLYGON_TYPE_LINES);
	CHECK_POLYGON_MODE(POLYGON_TYPE_TRIANGLES);
	CHECK_POLYGON_MODE(POLYGON_TYPE_TRIANGLE_STRIPS);
	return "";

#undef CHECK_POLYGON_MODE
}

static std::string StencilOpToString(const ESTENCIL_OP StencilOp)
{
#define CHECK_STENCIL_OP(x) if (StencilOp == x) { return #x; }

	CHECK_STENCIL_OP(STENCIL_OP_KEEP);
	CHECK_STENCIL_OP(STENCIL_OP_ZERO);
	CHECK_STENCIL_OP(STENCIL_OP_REPLACE);
	CHECK_STENCIL_OP(STENCIL_OP_INCR_SAT);
	CHECK_STENCIL_OP(STENCIL_OP_DECR_SAT);
	CHECK_STENCIL_OP(STENCIL_OP_INVERT);
	CHECK_STENCIL_OP(STENCIL_OP_INCR);
	CHECK_STENCIL_OP(STENCIL_OP_DECR);
	return "";

#undef CHECK_STENCIL_OP
}

void DoCompileGraphics(const std::string& SourceFile, const std::string& SearchedFolder, std::string& DstCompiledFile)
{
	std::cout << "Compiling file: " << SourceFile << std::endl;
	
	FULL_PIPELINE_DESCRIPTOR FullDesc = LoadGraphicsPipeline(SearchedFolder + '/' + SourceFile);
	
	std::cout << "Loaded the graphics pipeline" << std::endl;

	{
		json Output;

		Output["VertexShader"] = SerializeShader(&FullDesc.VS);
		Output["PixelShader"] = SerializeShader(&FullDesc.PS);
		if (HasHullShader(FullDesc))
		{
			Output["HullShader"] = SerializeShader(&FullDesc.HS);
		}
		if (HasDomainShader(FullDesc))
		{
			Output["DomainShader"] = SerializeShader(&FullDesc.DS);
		}
		if (HasGeometryShader(FullDesc))
		{
			Output["GeometryShader"] = SerializeShader(&FullDesc.GS);
		}
	
		std::ofstream OutputFile(DstCompiledFile);
		OutputFile << Output.dump(4);
	}
	
	{
		json InputLayout = json::array();
		
		for (uint32_t i = 0; i < FullDesc.InputLayout.InputItems.size(); i++)
		{
			auto& Input = FullDesc.InputLayout.InputItems[i];
			json SavedInput;
			SavedInput["Idx"]		= i;
			SavedInput["Name"]		= Input.Name;
			SavedInput["Format"]	= ItemFormatToString(Input.ItemFormat);
			InputLayout.push_back(SavedInput);
		}

		json RasterDesc;
		RasterDesc["bFillSolid"]				= (FullDesc.RasterDesc.bFillSolid);
		RasterDesc["bCull"]						= (FullDesc.RasterDesc.bCull);
		RasterDesc["bIsCounterClockwiseForward"] = (FullDesc.RasterDesc.bIsCounterClockwiseForward);
		RasterDesc["bDepthClipEnable"]			= (FullDesc.RasterDesc.bDepthClipEnable);
		RasterDesc["bAntialiasedLineEnabled"]	= (FullDesc.RasterDesc.bAntialiasedLineEnabled);
		RasterDesc["bMultisampleEnable"]		= (FullDesc.RasterDesc.bMultisampleEnable);
		RasterDesc["DepthBiasClamp"]			= (FullDesc.RasterDesc.DepthBiasClamp);
		RasterDesc["SlopedScaledDepthBias"]		= (FullDesc.RasterDesc.SlopeScaledDepthBias);
		RasterDesc["MultisampleLevel"]			= (MultiSamplingLevelToString(FullDesc.RasterDesc.MultisampleLevel));

		json RtvDescs = json::array();
		for (uint32_t i = 0; i < FullDesc.NumRenderTargets; i++)
		{
			GFX_RENDER_TARGET_DESC Input = FullDesc.RtvDescs[i];
			json Obj;
			Obj["bBlendEnable"]		= (Input.bBlendEnable);
			Obj["bLogicOpEnable"]	= (Input.bLogicOpEnable);
			Obj["SrcBlend"]			= (BlendStyleToString(Input.SrcBlend));
			Obj["DstBlend"]			= (BlendStyleToString(Input.DstBlend));
			Obj["BlendOp"]			= (BlendOpToString(Input.BlendOp));
			Obj["SrcBlendAlpha"]	= (BlendStyleToString(Input.SrcBlendAlpha));
			Obj["DstBlendAlpha"]	= (BlendStyleToString(Input.DstBlendAlpha));
			Obj["AlphaBlendOp"]		= (BlendOpToString(Input.AlphaBlendOp));
			Obj["LogicOp"]			= (LogicOpToString(Input.LogicOp));
			Obj["Format"]			= (FormatToString(Input.Format));
			RtvDescs.push_back(Obj);
		}
		
		auto Depth = FullDesc.DepthStencilState;
		json DepthFrontFace;
		DepthFrontFace["StencilFailOp"]			= (StencilOpToString(Depth.FrontFace.StencilFailOp));
		DepthFrontFace["StencilDepthFailOp"]	= (StencilOpToString(Depth.FrontFace.StencilDepthFailOp));
		DepthFrontFace["StencilPassOp"]			= (StencilOpToString(Depth.FrontFace.StencilPassOp));
		DepthFrontFace["ComparisonFunction"]	= (ComparisonFunctionToString(Depth.FrontFace.ComparisonFunction));
		json DepthBackFace;
		DepthBackFace["StencilFailOp"]			= (StencilOpToString(Depth.BackFace.StencilFailOp));
		DepthBackFace["StencilDepthFailOp"]		= (StencilOpToString(Depth.BackFace.StencilDepthFailOp));
		DepthBackFace["StencilPassOp"]			= (StencilOpToString(Depth.BackFace.StencilPassOp));
		DepthBackFace["ComparisonFunction"]		= (ComparisonFunctionToString(Depth.BackFace.ComparisonFunction));

		json DepthStencilDesc;
		DepthStencilDesc["Format"]			= (FormatToString(Depth.Format));
		DepthStencilDesc["bDepthEnable"]	= (Depth.bDepthEnable);
		DepthStencilDesc["DepthWriteMask"]	= (Depth.DepthWriteMask);
		DepthStencilDesc["DepthFunction"]	= (ComparisonFunctionToString(Depth.DepthFunction));
		DepthStencilDesc["bStencilEnable"]	= (Depth.bStencilEnable);
		DepthStencilDesc["FrontFace"]		= (DepthFrontFace);
		DepthStencilDesc["BackFace"]		= (DepthBackFace);
		
		json PipelineDescriptor;
		PipelineDescriptor["NumConstantBuffers"]		= FullDesc.Counts.NumConstantBuffers;
		PipelineDescriptor["NumSamplers"]				= FullDesc.Counts.NumSamplers;
		PipelineDescriptor["NumShaderResourceViews"]	= FullDesc.Counts.NumShaderResourceViews;
		PipelineDescriptor["NumUnorderedAccessViews"]	= FullDesc.Counts.NumUnorderedAccessViews;
		PipelineDescriptor["Type"]						= "Graphics";
		PipelineDescriptor["ShaderReference"]			= DstCompiledFile;
		PipelineDescriptor["InputLayout"]				= (InputLayout);
		PipelineDescriptor["PolygonType"]				= (PolygonModeToString(FullDesc.PolygonType));
		PipelineDescriptor["RasterDesc"]				= (RasterDesc);
		PipelineDescriptor["bEnableAlphaToCoverage"]	= (FullDesc.bEnableAlphaToCoverage);
		PipelineDescriptor["bIndependentBlendEnable"]	= (FullDesc.bIndependentBlendEnable);
		PipelineDescriptor["RtvDescs"]					= (RtvDescs);
		PipelineDescriptor["DepthStencilState"]			= (DepthStencilDesc);
		PipelineDescriptor["NumRenderTargets"]			= (FullDesc.NumRenderTargets);
		RootObject[SourceFile] = PipelineDescriptor;
	}
}

void EnsureFileDirectoryExists(const std::string& FileName)
{
	auto EndOfDirectory = FileName.find_last_of("/");
	std::string Directory = FileName.substr(0, EndOfDirectory);
	std::cout << Directory << std::endl;
	std::filesystem::create_directories(Directory);
}

static void WriteJsonToFile(const std::string& FileName, json Json)
{
	std::ofstream OutputFile(FileName);
	OutputFile << Json;
	OutputFile.close();
}

static void DoCompileCompute(const std::string& SourceFile, const std::string& SearchedFolder, std::string& DstCompiledFile)
{
	COMPUTE_PIPELINE_DESC Pipeline = LoadComputePipeline(SearchedFolder + '/' + SourceFile);
	json PipelineDescriptor;
	PipelineDescriptor["NumConstantBuffers"] = Pipeline.Counts.NumConstantBuffers;
	PipelineDescriptor["NumSamplers"] = Pipeline.Counts.NumSamplers;
	PipelineDescriptor["NumShaderResourceViews"] = Pipeline.Counts.NumShaderResourceViews;
	PipelineDescriptor["NumUnorderedAccessViews"] = Pipeline.Counts.NumUnorderedAccessViews;
	PipelineDescriptor["Type"] = "Compute";
	PipelineDescriptor["ShaderReference"]	= DstCompiledFile;
	RootObject[SourceFile] = PipelineDescriptor;
	WriteJsonToFile(DstCompiledFile, SerializeShader(&Pipeline.CS));
}

static void DoCompileRaytracing(const std::string& SourceFile, const std::string& SearchedFolder, std::string& DstCompiledFile)
{
	RAYTRACING_PIPELINE_DESC Pipeline = LoadRaytracingPipeline(SearchedFolder + '/' + SourceFile);
	json PipelineDescriptor;
	PipelineDescriptor["NumConstantBuffers"] = Pipeline.Counts.NumConstantBuffers;
	PipelineDescriptor["NumSamplers"] = Pipeline.Counts.NumSamplers;
	PipelineDescriptor["NumShaderResourceViews"] = Pipeline.Counts.NumShaderResourceViews;
	PipelineDescriptor["NumUnorderedAccessViews"] = Pipeline.Counts.NumUnorderedAccessViews;
	PipelineDescriptor["bHasIntersection"] = Pipeline.bHasIntersection;
	PipelineDescriptor["bHasClosestHit"] = Pipeline.bHasClosestHit;
	PipelineDescriptor["bHasAnyHit"] = Pipeline.bHasAnyHit;
	PipelineDescriptor["Type"] = "Raytracing";
	PipelineDescriptor["ShaderReference"] = DstCompiledFile;
	RootObject[SourceFile] = PipelineDescriptor;
	WriteJsonToFile(DstCompiledFile, SerializeShader(&Pipeline.Library));
}

std::string GetFileExtension(const std::string& FileName)
{
	std::string Result;
	auto Loc = FileName.find_last_of('.');
	Result = FileName.substr(Loc, Result.size() - 1);
	return Result;
}


int main(int argc, char** argv)
{
	if (argc != 4) 
	{
		std::cout << "Usage: ./d3d-shader-compiler <Dx12|Dx11> <Source Folder> <Destination Folder>" << std::endl;
		return 1;
	}

	std::string PlatformDest = argv[1];
	std::string SrcFolder = argv[2];
	std::string DstFolder = argv[3];
	
	std::vector<std::string> AllFiles = GetAllFilesInFolder(SrcFolder);

	if (PlatformDest != "Dx12" && PlatformDest != "Dx11")
	{
		std::cout << "First argument MUST be one of \"Dx12\" \"Dx11\"" << std::endl;
		return 1;
	}

	// Here we would check "PlatformDest" for "Vulkan" if and when SPIRV gets added to this project
	// For now because we're only going to assume DXIL, we'll just hardcode this
	SetCompilationMode(DXIL);

	std::string AdditionalCompiledLocation;
	if (PlatformDest == "Dx12" || PlatformDest == "Dx11")
	{
		AdditionalCompiledLocation = "DXILCompiledShaders/";
	}

	EnsureFileDirectoryExists(DstFolder);

	for (auto& File : AllFiles)
	{
		std::string FileExtension = GetFileExtension(File);
		std::cout << File << std::endl;

		std::string CompiledDest = DstFolder + AdditionalCompiledLocation + File;

		EnsureFileDirectoryExists(DstFolder + AdditionalCompiledLocation);
		
		bool bIsGraphics = false;
		if (FileExtension == ".gfx")
		{
			DoCompileGraphics(File, SrcFolder, CompiledDest);
		}
		else if (FileExtension == ".cmpt")
		{
			DoCompileCompute(File, SrcFolder, CompiledDest);
		}
		else if (FileExtension == ".ray")
		{
			DoCompileRaytracing(File, SrcFolder, CompiledDest);
		}
		else
		{
			// skip the file 
			continue;
		}
	}
	std::ofstream MetaFileDump(DstFolder + "ShaderPipelines.json");
	if (MetaFileDump.is_open())
	{
		MetaFileDump << RootObject.dump(4);
	}
	MetaFileDump.close();

	return 0;
}
