#include "d3d-shader-loader-helper.h"
#include "d3d-shader-loader-base64.h"
#include <cstdarg>
#include <fstream>


static ID3DShaderLoaderPrintHandler* g_PrintHandler;


void ID3DShaderLoaderPrintHandler::Error(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->ErrorImpl(buffer);
}

void ID3DShaderLoaderPrintHandler::Warn(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->WarnImpl(buffer);
}

void ID3DShaderLoaderPrintHandler::Message(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->MessageImpl(buffer);
}

namespace LoaderPriv {

	// Stupid redundant code. But I want logging without assuming
	// that g_PrintHandler isn't null
	static void Message(const char* fmt, ...)
	{
		if (g_PrintHandler == nullptr)
		{
			return;
		}

		char buffer[4096] = { };
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, 4096, fmt, args);
		va_end(args);

		g_PrintHandler->MessageImpl(buffer);
	}

	static void Warn(const char* fmt, ...)
	{
		if (g_PrintHandler == nullptr)
		{
			return;
		}

		char buffer[4096] = { };
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, 4096, fmt, args);
		va_end(args);

		g_PrintHandler->WarnImpl(buffer);
	}

	static void Error(const char* fmt, ...)
	{
		if (g_PrintHandler == nullptr)
		{
			return;
		}

		char buffer[4096] = { };
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, 4096, fmt, args);
		va_end(args);

		g_PrintHandler->ErrorImpl(buffer);
	}

	void SetPrintHandler(ID3DShaderLoaderPrintHandler* handler)
	{
		g_PrintHandler = handler;
	}

	static D3D12_BLEND D3D12BlendTypes[19] = {
			D3D12_BLEND_ZERO,
			D3D12_BLEND_ONE,
			D3D12_BLEND_SRC_COLOR,
			D3D12_BLEND_INV_SRC_COLOR,
			D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_DEST_ALPHA,
			D3D12_BLEND_INV_DEST_ALPHA,
			D3D12_BLEND_DEST_COLOR,
			D3D12_BLEND_INV_DEST_COLOR,
			D3D12_BLEND_SRC_ALPHA_SAT,
			D3D12_BLEND_BLEND_FACTOR,
			D3D12_BLEND_INV_BLEND_FACTOR,
			D3D12_BLEND_SRC1_COLOR,
			D3D12_BLEND_INV_SRC1_COLOR,
			D3D12_BLEND_SRC1_ALPHA,
			D3D12_BLEND_INV_SRC1_ALPHA
	};

	static D3D12_BLEND_OP D3D12BlendOperations[5] = {
		D3D12_BLEND_OP_ADD,
		D3D12_BLEND_OP_SUBTRACT,
		D3D12_BLEND_OP_REV_SUBTRACT,
		D3D12_BLEND_OP_MIN,
		D3D12_BLEND_OP_MAX
	};

	static D3D12_LOGIC_OP D3D12LogicOperations[16] = {
		D3D12_LOGIC_OP_CLEAR,
		D3D12_LOGIC_OP_SET,
		D3D12_LOGIC_OP_COPY,
		D3D12_LOGIC_OP_COPY_INVERTED,
		D3D12_LOGIC_OP_NOOP,
		D3D12_LOGIC_OP_INVERT,
		D3D12_LOGIC_OP_AND,
		D3D12_LOGIC_OP_NAND,
		D3D12_LOGIC_OP_OR,
		D3D12_LOGIC_OP_NOR,
		D3D12_LOGIC_OP_XOR,
		D3D12_LOGIC_OP_EQUIV,
		D3D12_LOGIC_OP_AND_REVERSE,
		D3D12_LOGIC_OP_AND_INVERTED,
		D3D12_LOGIC_OP_OR_REVERSE,
		D3D12_LOGIC_OP_OR_INVERTED
	};

	static D3D12_COMPARISON_FUNC D3D12ComparisonFunctions[8] = {
		D3D12_COMPARISON_FUNC_NEVER,
		D3D12_COMPARISON_FUNC_LESS,
		D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER,
		D3D12_COMPARISON_FUNC_NOT_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,
		D3D12_COMPARISON_FUNC_ALWAYS
	};

	static D3D12_STENCIL_OP D3D12StencilOperations[8] = {
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_ZERO,
		D3D12_STENCIL_OP_REPLACE,
		D3D12_STENCIL_OP_INCR_SAT,
		D3D12_STENCIL_OP_DECR_SAT,
		D3D12_STENCIL_OP_INVERT,
		D3D12_STENCIL_OP_INCR,
		D3D12_STENCIL_OP_DECR
	};


	static D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12PrimitiveTopologyTypes[4] = {
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  // Duplicate because d3d12 is weird about this
	};

	D3D12_BLEND D3D12_TranslateBlendType(EBLEND_STYLE Style)
	{
		return D3D12BlendTypes[(uint32_t)Style];
	}

	D3D12_BLEND_OP D3D12_TranslateBlendOp(EBLEND_OP BlendOp)
	{
		return D3D12BlendOperations[(uint32_t)BlendOp];
	}

	D3D12_LOGIC_OP D3D12_TranslateLogicOp(ELOGIC_OP InLogicOp)
	{
		return D3D12LogicOperations[(uint32_t)InLogicOp];
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12_TranslatePolygonType(EPOLYGON_TYPE PolygonType)
	{
		return D3D12PrimitiveTopologyTypes[(uint32_t)PolygonType];
	}

	D3D12_COMPARISON_FUNC D3D12_TranslateComparisonFunc(ECOMPARISON_FUNCTION InFunc)
	{
		return D3D12ComparisonFunctions[(uint32_t)InFunc];
	}

	D3D12_STENCIL_OP D3D12_TranslateStencilOp(ESTENCIL_OP InOp)
	{
		return D3D12StencilOperations[(uint32_t)InOp];
	}

	DXGI_FORMAT D3D12_TranslateDataFormat(EINPUT_ITEM_FORMAT Format)
	{
		switch (Format)
		{
		case INPUT_ITEM_FORMAT_FLOAT: return DXGI_FORMAT_R32_FLOAT;
		case INPUT_ITEM_FORMAT_INT: return DXGI_FORMAT_R32_SINT;
		case INPUT_ITEM_FORMAT_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
		case INPUT_ITEM_FORMAT_INT2: return DXGI_FORMAT_R32G32_SINT;
		case INPUT_ITEM_FORMAT_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case INPUT_ITEM_FORMAT_INT3: return DXGI_FORMAT_R32G32B32_SINT;
		case INPUT_ITEM_FORMAT_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case INPUT_ITEM_FORMAT_INT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	D3D12_INPUT_LAYOUT_DESC D3D12_TranslateInputLayout(const GFX_INPUT_LAYOUT_DESC& InDesc)
	{
		D3D12_INPUT_LAYOUT_DESC Result;

		D3D12_INPUT_ELEMENT_DESC* ResultItems = new D3D12_INPUT_ELEMENT_DESC[InDesc.InputItems.size()]{ };

		for (uint32_t i = 0; i < InDesc.InputItems.size(); i++)
		{
			const GFX_INPUT_ITEM_DESC& Item = InDesc.InputItems[i];
			ResultItems[i].SemanticName = new char[Item.Name.size()];
			memcpy((void*)ResultItems[i].SemanticName, Item.Name.c_str(), static_cast<uint32_t>(Item.Name.size() + 1));
			ResultItems[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			ResultItems[i].InputSlot = 0;
			ResultItems[i].Format = D3D12_TranslateDataFormat(InDesc.InputItems[i].ItemFormat);
			ResultItems[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			ResultItems[i].InstanceDataStepRate = 0;
		}

		Result.pInputElementDescs = ResultItems;
		Result.NumElements = static_cast<uint32_t>(InDesc.InputItems.size());
		return Result;
	}

	// TODO: Actually fill out this function?
	DXGI_SAMPLE_DESC D3D_TranslateMultisampleLevel(EMULTISAMPLE_LEVEL MultisampleLevel)
	{
		(void)(MultisampleLevel);
		DXGI_SAMPLE_DESC RetDesc = { };
#if 0
		switch (MultisampleLevel)
		{
		case MULTISAMPLE_LEVEL_0:
			break;
		case MULTISAMPLE_LEVEL_4X:
			RetDesc.Count = 3;
			RetDesc.Quality = 4;
			break;
		case MULTISAMPLE_LEVEL_8X:
			RetDesc.Count = 7;
			RetDesc.Quality = 8;
			break;
		case MULTISAMPLE_LEVEL_16X:
			RetDesc.Count = 15;
			RetDesc.Quality = 16;
			break;
		}
#endif
		RetDesc.Count = 1;
		RetDesc.Quality = 0;
		return RetDesc;
	}
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12_TranslateGfxDesc(const GFX_PIPELINE_STATE_DESC& desc)
	{
		// RootSignature MUST be set by the caller of this function
		D3D12_GRAPHICS_PIPELINE_STATE_DESC result = { };

		// Set shaders
		result.VS.pShaderBytecode = (void*)desc.VS.data();
		result.VS.BytecodeLength = desc.VS.size();

		result.PS.pShaderBytecode = (void*)desc.PS.data();
		result.PS.BytecodeLength = desc.PS.size();

		if (desc.DS.size() > 0)
		{
			result.DS.pShaderBytecode = (void*)desc.DS.data();
			result.DS.BytecodeLength = desc.DS.size();
		}

		if (desc.HS.size() > 0)
		{
			result.HS.pShaderBytecode = (void*)desc.HS.data();
			result.HS.BytecodeLength = desc.HS.size();
		}

		if (desc.GS.size() > 0)
		{
			result.GS.pShaderBytecode = (void*)desc.GS.data();
			result.GS.BytecodeLength = desc.GS.size();
		}

		// Set rendertargets
		for (uint32_t i = 0; i < desc.NumRenderTargets; i++)
		{
			result.BlendState.RenderTarget[i].BlendEnable = desc.RtvDescs[i].bBlendEnable;
			result.BlendState.RenderTarget[i].LogicOpEnable = desc.RtvDescs[i].bLogicOpEnable;
			result.BlendState.RenderTarget[i].SrcBlend = D3D12_TranslateBlendType(desc.RtvDescs[i].SrcBlend);
			result.BlendState.RenderTarget[i].DestBlend = D3D12_TranslateBlendType(desc.RtvDescs[i].DstBlend);
			result.BlendState.RenderTarget[i].BlendOp = D3D12_TranslateBlendOp(desc.RtvDescs[i].BlendOp);
			result.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_TranslateBlendType(desc.RtvDescs[i].SrcBlendAlpha);
			result.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_TranslateBlendType(desc.RtvDescs[i].DstBlendAlpha);
			result.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_TranslateBlendOp(desc.RtvDescs[i].AlphaBlendOp);
			result.BlendState.RenderTarget[i].LogicOp = D3D12_TranslateLogicOp(desc.RtvDescs[i].LogicOp);
			result.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			result.RTVFormats[i] = D3D_TranslateFormat(desc.RtvDescs[i].Format);
		}

		result.DSVFormat = D3D_TranslateFormat(desc.DepthStencilState.Format);
		result.SampleMask = 0xffffffff;
		result.NumRenderTargets = desc.NumRenderTargets;

		if (desc.RasterDesc.bFillSolid)
		{
			result.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		}
		else 
		{
			result.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		}

		if (desc.RasterDesc.bCull)
		{
			result.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		}
		else
		{
			result.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		}

		result.RasterizerState.FrontCounterClockwise = desc.RasterDesc.bIsCounterClockwiseForward;
		result.RasterizerState.DepthBias = 0;
		result.RasterizerState.DepthBiasClamp = desc.RasterDesc.DepthBiasClamp;
		result.RasterizerState.DepthClipEnable = desc.RasterDesc.bDepthClipEnable;
		result.RasterizerState.MultisampleEnable = desc.RasterDesc.bMultisampleEnable;
		result.RasterizerState.ForcedSampleCount = 0;
		result.RasterizerState.AntialiasedLineEnable = desc.RasterDesc.bAntialiasedLineEnabled;

		if (desc.InputLayout.InputItems.size() > 0)
		{
			result.InputLayout = D3D12_TranslateInputLayout(desc.InputLayout);
		}

		result.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		result.PrimitiveTopologyType = D3D12_TranslatePolygonType(desc.PolygonType);

		result.SampleDesc = D3D_TranslateMultisampleLevel(desc.RasterDesc.MultisampleLevel);

		if (desc.DepthStencilState.bDepthEnable) 
		{
			result.DepthStencilState.DepthEnable = desc.DepthStencilState.bDepthEnable;
			result.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			result.DepthStencilState.DepthFunc = D3D12_TranslateComparisonFunc(desc.DepthStencilState.DepthFunction);
		}

		if (desc.DepthStencilState.bStencilEnable)
		{
			result.DepthStencilState.StencilEnable = desc.DepthStencilState.bStencilEnable;
			result.DepthStencilState.StencilReadMask = 0xff;
			result.DepthStencilState.StencilWriteMask = 0xff;

			D3D12_DEPTH_STENCILOP_DESC frontFace = { };
			D3D12_DEPTH_STENCILOP_DESC backFace = { };

			frontFace.StencilFailOp = D3D12_TranslateStencilOp(desc.DepthStencilState.FrontFace.StencilFailOp);
			frontFace.StencilDepthFailOp = D3D12_TranslateStencilOp(desc.DepthStencilState.FrontFace.StencilDepthFailOp);
			frontFace.StencilPassOp = D3D12_TranslateStencilOp(desc.DepthStencilState.FrontFace.StencilPassOp);
			frontFace.StencilFunc = D3D12_TranslateComparisonFunc(desc.DepthStencilState.FrontFace.ComparisonFunction);

			backFace.StencilFailOp = D3D12_TranslateStencilOp(desc.DepthStencilState.BackFace.StencilFailOp);
			backFace.StencilDepthFailOp = D3D12_TranslateStencilOp(desc.DepthStencilState.BackFace.StencilDepthFailOp);
			backFace.StencilPassOp = D3D12_TranslateStencilOp(desc.DepthStencilState.BackFace.StencilPassOp);
			backFace.StencilFunc = D3D12_TranslateComparisonFunc(desc.DepthStencilState.BackFace.ComparisonFunction);

			result.DepthStencilState.FrontFace = frontFace;
			result.DepthStencilState.BackFace = backFace;
		}

		return result;
	}

	void FreeD3D12GraphicsPipelineDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		for (uint32_t i = 0; i < desc.InputLayout.NumElements; i++)
		{
			delete[] desc.InputLayout.pInputElementDescs[i].SemanticName;
		}

		delete[] desc.InputLayout.pInputElementDescs;
	}

	D3D12_TEXTURE_ADDRESS_MODE D3D12_TranslateWrapMode(EWRAP_MODE WrapMode)
	{
		switch (WrapMode)
		{
		case WRAP_MODE_LOOP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case WRAP_MODE_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case WRAP_MODE_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		}
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}


	D3D12_SAMPLER_DESC D3D12_TranslateSamplerDesc(const SAMPLER_DESC& InDesc)
	{
		float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		D3D12_SAMPLER_DESC Result = { };
		Result.AddressU = D3D12_TranslateWrapMode(InDesc.UAddress);
		Result.AddressV = D3D12_TranslateWrapMode(InDesc.VAddress);
		Result.AddressW = D3D12_TranslateWrapMode(InDesc.WAddress);
		memcpy(Result.BorderColor, white, sizeof(float) * 4);
		Result.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		Result.MipLODBias = 0.0f;
		Result.MaxAnisotropy = 16;
		Result.MinLOD = 0.0f;
		Result.MaxLOD = D3D12_FLOAT32_MAX;
		Result.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		return Result;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC D3D12_TranslateCmptDesc(const COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC result = { };
		result.CS.pShaderBytecode = (void*)desc.CS.data();
		result.CS.BytecodeLength = desc.CS.size();
		return result;
	}

	static bool LoadShaderByteCode(std::filesystem::path fullPath, COMPUTE_PIPELINE_STATE_DESC& desc, CompilerFlags flags)
	{
		std::ifstream file(fullPath.native());

		if (!file.is_open())
		{
			Error("Failed to open shader file %s for reading", fullPath.c_str());
			return false;
		}

		nlohmann::json fileData = nlohmann::json::parse(file);

		std::string encodedData = fileData[CompilerFlagsToStr(flags)];

		std::vector<uint8_t> res = FromBase64(encodedData);

		if (res.size() == 0)
		{
			Error("Failed to decode data in %s", fullPath.c_str());
			return false;
		}

		std::swap(res, desc.CS);

		return true;
	}

	static bool LoadByteCode(nlohmann::json& json, CompilerFlags flags, ShaderByteCode& outCode)
	{
		std::string encoded = json[CompilerFlagsToStr(flags)];

		std::vector<uint8_t> res = FromBase64(encoded);

		if (res.size() == 0)
		{
			return false;
		}

		std::swap(res, outCode);

		return true;
	}

	static bool LoadShaderByteCode(std::filesystem::path fullPath, GFX_PIPELINE_STATE_DESC& desc, CompilerFlags flags)
	{
		std::ifstream file(fullPath.native());

		if (!file.is_open())
		{
			Error("Failed to open shader file %s for reading", fullPath.c_str());
			return false;
		}

		nlohmann::json fileData = nlohmann::json::parse(file);

		if (fileData.contains("VertexShader"))
		{
			if (!LoadByteCode(fileData["VertexShader"], flags, desc.VS))
			{
				Error("Failed to decode data in %s", fullPath.c_str());
				return false;
			}
		}
		else
		{
			Error("Graphics pipelines require a vertex shader");
			return false;
		}

		if (fileData.contains("PixelShader"))
		{
			if (!LoadByteCode(fileData["PixelShader"], flags, desc.VS))
			{
				Error("Failed to decode data in %s", fullPath.c_str());
				return false;
			}
		}
		else
		{
			Error("Graphics pipelines require a pixel shader");
			return false;
		}

		if (fileData.contains("GeometryShader"))
		{
			if (!LoadByteCode(fileData["GeometryShader"], flags, desc.VS))
			{
				Error("Failed to decode data in %s", fullPath.c_str());
				return false;
			}
		}

		if (fileData.contains("HullShader"))
		{
			if (!LoadByteCode(fileData["HullShader"], flags, desc.VS))
			{
				Error("Failed to decode data in %s", fullPath.c_str());
				return false;
			}
		}

		if (fileData.contains("DomainShader"))
		{
			if (!LoadByteCode(fileData["DomainShader"], flags, desc.VS))
			{
				Error("Failed to decode data in %s", fullPath.c_str());
				return false;
			}
		}
	}

	bool LoadCmptDescFromJson(const nlohmann::json& json, COMPUTE_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags)
	{
		if (json["Type"].get<std::string>() != "Compute")
		{
			Message("Pipeline isn't compute");
			return false;
		}

		PIPELINE_STATE_RESOURCE_COUNTS counts = { };
		counts.NumConstantBuffers = static_cast<uint8_t>(json["NumConstantBuffers"].get<uint32_t>());
		counts.NumSamplers = static_cast<uint8_t>(json["NumSamplers"].get<uint32_t>());
		counts.NumShaderResourceViews = static_cast<uint8_t>(json["NumShaderResourceViews"].get<uint32_t>());
		counts.NumUnorderedAccessViews = static_cast<uint8_t>(json["NumUnorderedAccessViews"].get<uint32_t>());

		desc.Counts = counts;

		return LoadShaderByteCode(startPath / json["ShaderReference"], desc, flags);
	}

	bool LoadRTDescFromJson(const nlohmann::json& json, COMPUTE_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags)
	{
		if (json["Type"].get<std::string>() != "Raytracing")
		{
			Message("Pipeline isn't raytracing");
			return false;
		}

		PIPELINE_STATE_RESOURCE_COUNTS counts = { };
		counts.NumConstantBuffers = static_cast<uint8_t>(json["NumConstantBuffers"].get<uint32_t>());
		counts.NumSamplers = static_cast<uint8_t>(json["NumSamplers"].get<uint32_t>());
		counts.NumShaderResourceViews = static_cast<uint8_t>(json["NumShaderResourceViews"].get<uint32_t>());
		counts.NumUnorderedAccessViews = static_cast<uint8_t>(json["NumUnorderedAccessViews"].get<uint32_t>());

		desc.Counts = counts;

		return LoadShaderByteCode(startPath / json["ShaderReference"], desc, flags);
	}

	bool LoadGfxDescFromJson(const nlohmann::json& json, GFX_PIPELINE_STATE_DESC& desc, std::filesystem::path startPath, CompilerFlags flags)
	{
		if (json["Type"].get<std::string>() != "Graphics")
		{
			Message("Pipeline isn't graphics.");
			return false;
		}

		PIPELINE_STATE_RESOURCE_COUNTS counts = { };
		counts.NumConstantBuffers = static_cast<uint8_t>(json["NumConstantBuffers"].get<uint32_t>());
		counts.NumSamplers = static_cast<uint8_t>(json["NumSamplers"].get<uint32_t>());
		counts.NumShaderResourceViews = static_cast<uint8_t>(json["NumShaderResourceViews"].get<uint32_t>());
		counts.NumUnorderedAccessViews = static_cast<uint8_t>(json["NumUnorderedAccessViews"].get<uint32_t>());

		desc.Counts = counts;

		desc.NumRenderTargets = json["NumRenderTargets"].get<uint32_t>();
		if (desc.NumRenderTargets > 8)
		{
			return false;
		}

		if (!LoadGfxRasterDescFromJson(json["RasterDesc"], desc.RasterDesc))
		{
			return false;
		}

		if (!LoadGfxRTDescFromJson(json["RtvDescs"], desc.RtvDescs, desc.NumRenderTargets))
		{
			return false;
		}

		if (!LoadGfxInputLayoutFromJson(json["InputLayout"], desc.InputLayout))
		{
			return false;
		}

		if (!LoadGfxDepthStencilDescFromJson(json["DepthStencilState"], desc.DepthStencilState))
		{
			return false;
		}

		desc.PolygonType = ParsePolygonType(json["PolygonType"].get<std::string>());
		desc.bEnableAlphaToCoverage = json["bEnableAlphaToCoverage"].get<bool>();
		desc.bIndependentBlendEnable = json["bIndependentBlendEnable"].get<bool>();

		return LoadShaderByteCode(startPath / json["ShaderReference"], desc, flags);
	}

	bool LoadGfxRasterDescFromJson(const nlohmann::json& json, GFX_RASTER_DESC& desc)
	{
		desc.bFillSolid = json["bFillSolid"].get<bool>();
		desc.bCull = json["bCull"].get<bool>();
		desc.bIsCounterClockwiseForward = json["bIsCounterClockwiseForward"].get<bool>();
		desc.bDepthClipEnable = json["bDepthClipEnable"].get<bool>();
		desc.bAntialiasedLineEnabled = json["bAntialiasedLineEnabled"].get<bool>();
		desc.bMultisampleEnable = json["bMultisampleEnable"].get<bool>();
		desc.DepthBiasClamp = json["DepthBiasClamp"].get<float>();
		desc.SlopeScaledDepthBias = json["SlopedScaledDepthBias"].get<float>();
		desc.MultisampleLevel = ParseMultisampleLevel(json["MultisampleLevel"].get<std::string>());
		return true;
	}

	bool LoadGfxInputLayoutFromJson(const nlohmann::json& json, GFX_INPUT_LAYOUT_DESC& desc)
	{
		desc.InputItems.resize(static_cast<uint32_t>(json.size()));
		for (int i = 0; i < json.size(); i++)
		{
			nlohmann::json Element = json[i];
			std::string ItemName = Element["Name"].get<std::string>();
			EINPUT_ITEM_FORMAT ItemFormat = ParseItemFormat(Element["Format"].get<std::string>());
			int Idx = Element["Idx"].get<int>();
			desc.InputItems[Idx].Name = ItemName;
			desc.InputItems[Idx].ItemFormat = ItemFormat;
		}
		return true;
	}

	bool LoadGfxRTDescFromJson(const nlohmann::json& json, GFX_RENDER_TARGET_DESC* outDesc, uint32_t numRenderTargets)
	{
		for (uint32_t i = 0; i < numRenderTargets; i++)
		{
			nlohmann::json RtvDesc = json[i];
			GFX_RENDER_TARGET_DESC Rtv = { };
			Rtv.bBlendEnable = RtvDesc["bBlendEnable"].get<bool>();
			Rtv.bLogicOpEnable = RtvDesc["bLogicOpEnable"].get<bool>();
			Rtv.SrcBlend = ParseBlendStyle(RtvDesc["SrcBlend"].get<std::string>());
			Rtv.DstBlend = ParseBlendStyle(RtvDesc["DstBlend"].get<std::string>());
			Rtv.BlendOp = ParseBlendOp(RtvDesc["BlendOp"].get<std::string>());
			Rtv.SrcBlendAlpha = ParseBlendStyle(RtvDesc["SrcBlendAlpha"].get<std::string>());
			Rtv.DstBlendAlpha = ParseBlendStyle(RtvDesc["DstBlendAlpha"].get<std::string>());
			Rtv.AlphaBlendOp = ParseBlendOp(RtvDesc["AlphaBlendOp"].get<std::string>());
			Rtv.LogicOp = ParseLogicOp(RtvDesc["LogicOp"].get<std::string>());
			Rtv.Format = ParseFormat(RtvDesc["Format"].get<std::string>());
			outDesc[i] = Rtv;
		}

		return true;
	}

	bool LoadGfxDepthStencilDescFromJson(const nlohmann::json& json, GFX_DEPTH_STENCIL_DESC& desc)
	{
		desc.Format = ParseFormat(json["Format"].get<std::string>());
		desc.bDepthEnable = json["bDepthEnable"].get<bool>();
		desc.DepthWriteMask = json["DepthWriteMask"].get<uint32>();
		desc.DepthFunction = ParseComparisonFunction(json["DepthFunction"].get<std::string>());
		desc.bStencilEnable = json["bStencilEnable"].get<bool>();

		nlohmann::json JsonFrontFace = json["FrontFace"];
		desc.FrontFace.StencilFailOp = ParseStencilOp(JsonFrontFace["StencilFailOp"].get<std::string>());
		desc.FrontFace.StencilDepthFailOp = ParseStencilOp(JsonFrontFace["StencilDepthFailOp"].get<std::string>());
		desc.FrontFace.StencilPassOp = ParseStencilOp(JsonFrontFace["StencilPassOp"].get<std::string>());
		desc.FrontFace.ComparisonFunction = ParseComparisonFunction(JsonFrontFace["ComparisonFunction"].get<std::string>());

		nlohmann::json JsonBackFace = json["BackFace"];
		desc.BackFace.StencilFailOp = ParseStencilOp(JsonBackFace["StencilFailOp"].get<std::string>());
		desc.BackFace.StencilDepthFailOp = ParseStencilOp(JsonBackFace["StencilDepthFailOp"].get<std::string>());
		desc.BackFace.StencilPassOp = ParseStencilOp(JsonBackFace["StencilPassOp"].get<std::string>());
		desc.BackFace.ComparisonFunction = ParseComparisonFunction(JsonBackFace["ComparisonFunction"].get<std::string>());
		return true;
	}

}

