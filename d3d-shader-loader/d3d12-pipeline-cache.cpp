#include "d3d12-pipeline-cache.h"
#include <fstream>
#include <nlohmann.hpp>
#include <iostream>


class ShaderLoaderDefaultPrintHandler : public ID3DShaderLoaderPrintHandler
{
public:

	void ErrorImpl(const char* message) override
	{
		std::cout << "[ERROR] " << message << std::endl;
	}
	
	void WarnImpl(const char* message) override
	{
		std::cout << "[WARN] " << message << std::endl;
	}
	
	void MessageImpl(const char* message) override
	{
		std::cout << "[MSG] " << message << std::endl;
	}
};

static ShaderLoaderDefaultPrintHandler g_PrintHandler;


D3D12PipelineCache::D3D12PipelineCache(ID3D12Device* device) :
	m_rootSigLib(device),
	m_device(device),
	m_print(nullptr)
{
}

D3D12PipelineCache::~D3D12PipelineCache()
{
}

void D3D12PipelineCache::SetPrintHandler(ID3DShaderLoaderPrintHandler* handler)
{
	m_print = handler;
	m_rootSigLib.SetPrintHandler(handler);
	LoaderPriv::SetPrintHandler(handler);
}

bool D3D12PipelineCache::LoadDirectory(const std::filesystem::path& dirPath, CompilerFlags flags)
{
	if (m_print == nullptr)
	{
		SetPrintHandler(&g_PrintHandler);
	}

	CheckRaytracingSupport();

	if (!m_rootSigLib.BuildRootSignatures())
	{
		m_print->Error("Failed to build root signatures");
		return false;
	}

	std::filesystem::path shaderFile = dirPath / "ShaderPipelines.json";

	if (!std::filesystem::is_regular_file(shaderFile))
	{
		m_print->Error("%s does not exist, unable to load pipelines", shaderFile.c_str());
		return false;
	}

	std::ifstream metadataFile(shaderFile);
	nlohmann::json json = nlohmann::json::parse(metadataFile);

	for (auto entry = json.begin(); entry != json.end(); entry++)
	{
		auto data = entry.value();
		std::string type = data["Type"].get<std::string>();

		m_print->Message("Attempting to load pipeline %s", entry.key().c_str());

		if (type == "Graphics")
		{
			GFX_PIPELINE_STATE_DESC desc = { };
			if (!LoaderPriv::LoadGfxDescFromJson(data, desc, dirPath, flags))
			{
				m_print->Error("Failed to load pipeline");
				return false;
			}

			D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dDesc = LoaderPriv::D3D12_TranslateGfxDesc(desc);
			ID3D12RootSignature* rootSig = m_rootSigLib.FindBest(
				desc.Counts.NumConstantBuffers,
				desc.Counts.NumShaderResourceViews,
				desc.Counts.NumSamplers,
				desc.Counts.NumUnorderedAccessViews
			);

			if (rootSig == nullptr)
			{
				m_print->Error("Failed to assign a root signature to pipeline");
				return false;
			}

			d3dDesc.pRootSignature = rootSig;

			D3DPipeline newEntry = { };
			newEntry.RootSignature = rootSig;

			if (FAILED(m_device->CreateGraphicsPipelineState(&d3dDesc, IID_PPV_ARGS(&newEntry.PipelineState)))) 
			{
				m_print->Error("Failed to create ID3D12PipelineState object");
				return false;
			}

			m_library[entry.key()] = newEntry;
		}
		else if (type == "Compute")
		{
			COMPUTE_PIPELINE_STATE_DESC desc = { };
			if (!LoaderPriv::LoadCmptDescFromJson(data, desc, dirPath, flags))
			{
				m_print->Error("Failed to load pipeline");
				return false;
			}

			D3D12_COMPUTE_PIPELINE_STATE_DESC d3dDesc = LoaderPriv::D3D12_TranslateCmptDesc(desc);
			ID3D12RootSignature* rootSig = m_rootSigLib.FindBest(
				desc.Counts.NumConstantBuffers,
				desc.Counts.NumShaderResourceViews,
				desc.Counts.NumSamplers,
				desc.Counts.NumUnorderedAccessViews
			);

			if (rootSig == nullptr)
			{
				m_print->Error("Failed to assign a root signature to pipeline");
				return false;
			}

			d3dDesc.pRootSignature = rootSig;

			D3DPipeline newEntry = { };
			newEntry.RootSignature = rootSig;

			if (FAILED(m_device->CreateComputePipelineState(&d3dDesc, IID_PPV_ARGS(&newEntry.PipelineState))))
			{
				m_print->Error("Failed to create ID3D12PipelineState object");
				return false;
			}

			m_library[entry.key()] = newEntry;
		}
		else if (type == "Raytracing")
		{
			if (!m_hasRaytracingSupport)
			{
				m_print->Message("Skipping raytracing pipeline");
				continue;
			}

			COMPUTE_PIPELINE_STATE_DESC desc = { };
			if (!LoaderPriv::LoadCmptDescFromJson(data, desc, dirPath, flags))
			{
				m_print->Error("Failed to load pipeline");
				return false;
			}

			D3D12_STATE_OBJECT_DESC soDesc = { };
			soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
			
			
		}
	}
}

bool D3D12PipelineCache::FindPipeline(const std::string& name, D3DPipeline* outPipeline)
{
	return false;
}

void D3D12PipelineCache::CheckRaytracingSupport()
{
	ID3D12Device5* device5 = nullptr;
	m_device->QueryInterface(&device5);

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = { };
	device5->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options));

	m_hasRaytracingSupport = true;
	if (options.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		m_hasRaytracingSupport = false;
	}

	device5->Release();
}
