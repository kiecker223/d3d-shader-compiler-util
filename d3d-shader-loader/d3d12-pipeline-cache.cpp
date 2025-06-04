#include "d3d12-pipeline-cache.h"
#include <string.h>
#include <string>
#include <fstream>
#include <nlohmann.hpp>
#include <iostream>
#include <vector>


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


static const char s_RayGenFuncName[] = "RayGen";
static const char s_ClosestHitName[] = "ClosestHit";
static const char s_HitGroupName[] = "HitGroup";
static const char s_MissName[] = "Miss";


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

static void FreeD3DStateObjectDesc(D3D12_STATE_OBJECT_DESC& desc);

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

			RAYTRACING_PIPELINE_STATE_DESC desc = { };
			if (!LoaderPriv::LoadRTDescFromJson(data, desc, dirPath, flags))
			{
				m_print->Error("Failed to load pipeline");
				return false;
			}

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

static void FreeD3DStateObjectDesc(D3D12_STATE_OBJECT_DESC& desc)
{
	for (uint32_t i = 0; i < desc.NumSubobjects; i++)
	{
		switch (desc.pSubobjects[i].Type)
		{
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
		{
			D3D12_DXIL_LIBRARY_DESC* lib = (D3D12_DXIL_LIBRARY_DESC*)desc.pSubobjects[i].pDesc;
			delete[] lib->pExports;
			delete lib;
		} break;
		case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
		{
			D3D12_HIT_GROUP_DESC* hg = (D3D12_HIT_GROUP_DESC*)desc.pSubobjects[i].pDesc;
			delete[] hg->HitGroupExport;
			delete hg;
		} break;

		}
	}
	
	delete desc.pSubobjects;
}

void D3D12PipelineCache::BuildDxrStateDesc(const RAYTRACING_PIPELINE_STATE_DESC& desc, const std::string& shaderName, D3D12_STATE_OBJECT_DESC& outDesc)
{
	(void)desc;
	(void)shaderName;
	(void)outDesc;
	return;
	D3D12_SHADER_BYTECODE libraryCode = { };
	libraryCode.pShaderBytecode = (void*)desc.Library.data();
	libraryCode.BytecodeLength = desc.Library.size();

	std::vector<D3D12_STATE_SUBOBJECT> subObjects;

	// -- DXIL Library subobject
	{
		/*
		* RayGen <- required
		* RayClosestHit <- not required / Should this be required?
		* RayIntersect <- not required
		* RayAnyHit <- not required
		* RayMiss <- required
		*/

		D3D12_EXPORT_DESC* exports = new D3D12_EXPORT_DESC[5]{ };
		uint32_t i = 2;
		exports[0].Name = L"RayGen";
		exports[1].Name = L"RayMiss";
		if (desc.bHasAnyHit)
		{
			exports[i].Name = L"RayAnyHit";
			i++;
		}
		if (desc.bHasClosestHit)
		{
			exports[i].Name = L"RayClosestHit";
			i++;
		}
		if (desc.bHasIntersection)
		{
			exports[i].Name = L"RayIntersection";
			i++;
		}


		D3D12_DXIL_LIBRARY_DESC* library = new D3D12_DXIL_LIBRARY_DESC{};
		library->DXILLibrary = libraryCode;
		library->pExports = exports;
		library->NumExports = i;

		D3D12_STATE_SUBOBJECT subObject = { };
		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		subObject.pDesc = (void*)library;
		subObjects.push_back(subObject);
	}

	// -- Triangle hit groups
	{
		D3D12_HIT_GROUP_DESC* hitGroup = new D3D12_HIT_GROUP_DESC{ };
		
		/*    
		_In_opt_  LPCWSTR ClosestHitShaderImport;
		_In_opt_  LPCWSTR IntersectionShaderImport;
		_In_opt_  LPCWSTR AnyHitShaderImport;
		*/

		if (desc.bHasAnyHit)
		{
			hitGroup->AnyHitShaderImport = L"RayAnyHit";
		}
		if (desc.bHasIntersection)
		{
			hitGroup->IntersectionShaderImport = L"RayIntersection";
		}
		if (desc.bHasClosestHit)
		{
			hitGroup->ClosestHitShaderImport = L"RayClosestHit";
		}

		hitGroup->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		std::wstring groupExport(shaderName.begin(), shaderName.end());

		hitGroup->HitGroupExport = new wchar_t[groupExport.length() + 1] { 0 };
		memcpy((void*)hitGroup->HitGroupExport, 
			   (void*)groupExport.c_str(), 
			   groupExport.length() * sizeof(wchar_t)
		);

		D3D12_STATE_SUBOBJECT subObject = { };
		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = hitGroup;

		subObjects.push_back(subObject);
	}
	{

	}
	
}

