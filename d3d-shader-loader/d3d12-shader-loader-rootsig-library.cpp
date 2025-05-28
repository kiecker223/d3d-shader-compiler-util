#include "d3d12-shader-loader-rootsig-library.h"
#include "D3D12Helper.h"


namespace LoaderPriv {

	static void CreateRootSignatureDesc(D3D12_ROOT_SIGNATURE_DESC* desc, uint8_t numCbvs, uint8_t numSmps, uint8_t numSrvs, uint8_t numUavs)
	{
		uint32_t numParams = numCbvs + (numSrvs >= 1 ? 1 : 0) + (numSmps >= 1 ? 1 : 0) + (numSrvs >= 1 ? 1 : 0) + (numUavs >= 1 ? 1 : 0);
		D3D12_ROOT_PARAMETER* rootParams = new D3D12_ROOT_PARAMETER[numParams]{ };

		uint32_t i = 0;

		for (; i < numCbvs; i++)
		{
			CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(rootParams[i], i);
		}
		if (numSrvs > 0)
		{
			D3D12_DESCRIPTOR_RANGE* range = new D3D12_DESCRIPTOR_RANGE();
			CD3DX12_DESCRIPTOR_RANGE::Init(*range, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSrvs, 0);
			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParams[i].DescriptorTable.pDescriptorRanges = range;
			rootParams[i].DescriptorTable.NumDescriptorRanges = 1;
			i++;
		}
		if (numSmps > 0)
		{
			D3D12_DESCRIPTOR_RANGE* range = new D3D12_DESCRIPTOR_RANGE();
			CD3DX12_DESCRIPTOR_RANGE::Init(*range, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSrvs, 0);
			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParams[i].DescriptorTable.pDescriptorRanges = range;
			rootParams[i].DescriptorTable.NumDescriptorRanges = 1;
			i++;
		}
		if (numUavs > 0)
		{
			D3D12_DESCRIPTOR_RANGE* range = new D3D12_DESCRIPTOR_RANGE();
			CD3DX12_DESCRIPTOR_RANGE::Init(*range, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numSrvs, 0);
			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParams[i].DescriptorTable.pDescriptorRanges = range;
			rootParams[i].DescriptorTable.NumDescriptorRanges = 1;
			i++;
		}

		desc->pParameters = rootParams;
		desc->NumParameters = numParams;
		desc->Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	static void FreeRootSignatureDesc(D3D12_ROOT_SIGNATURE_DESC* desc)
	{
		for (uint32_t i = 0; i < desc->NumParameters; i++)
		{
			if (desc->pParameters[i].DescriptorTable.pDescriptorRanges != nullptr)
			{
				delete desc->pParameters[i].DescriptorTable.pDescriptorRanges;
			}
		}

		delete[] desc->pParameters;
	}

	static uint32_t NextPowerOfTwo(uint32_t in)
	{
		in--;
		in |= in >> 1;
		in |= in >> 2;
		in |= in >> 4;
		in |= in >> 8;
		in |= in >> 16;
		in++;
		return in;
	}

	RootSignatureTier ShaderResourceCounts::Quantize() const
	{
		uint8_t quantized = NextPowerOfTwo(max(NumSRVs, max(NumUAVs, max(NumSamplers, NumCBVs))));

		if (NumUAVs > 0)
		{
			switch (quantized) 
			{
				case 1:  return TIER_1CBV_1SRV_1SAMPLER;
				case 2:  return TIER_2CBV_2SRV_2SAMPLER;
				case 4:	 return TIER_4CBV_4SRV_4SAMPLER;
				case 8:	 return TIER_8CBV_8SRV_8SAMPLER;
				case 16: return TIER_16CBV_16SRV_16SAMPLER;
			}
		}
		else
		{
			switch (quantized)
			{
				case 1: return TIER_1CBV_1SRV_1UAV_1SAMPLER;
				case 2: return TIER_2CBV_2SRV_2UAV_2SAMPLER;
				case 4: return TIER_4CBV_4SRV_4UAV_4SAMPLER;
				case 8: return TIER_8CBV_8SRV_8UAV_8SAMPLER;
				case 16: return TIER_16CBV_16SRV_16UAV_16SAMPLER;
			}
		}
		return RootSignatureTier::INVALID_TIER;
	}
	
	D3D12RootSignatureLibrary::D3D12RootSignatureLibrary(ID3D12Device* device) :
		m_device(device)
	{
	}
	
	D3D12RootSignatureLibrary::~D3D12RootSignatureLibrary()
	{
		m_device = nullptr;
		m_handler = nullptr;

		for (uint32_t i = 0; i < (uint32_t)RootSignatureTier::NUM_TIERS; i++)
		{
			m_rootSigs[i]->Release();
		}
	}

	void D3D12RootSignatureLibrary::SetPrintHandler(ID3DShaderLoaderPrintHandler* handler)
	{
		m_handler = handler;
	}
	
	bool D3D12RootSignatureLibrary::BuildRootSignatures()
	{
		for (uint32_t i = 0; i < (uint32_t)RootSignatureTier::NUM_TIERS; i++)
		{
			if (!CreateRootSignature((RootSignatureTier)i))
			{
				return false;
			}
		}

		return true;
	}
	
	ID3D12RootSignature* D3D12RootSignatureLibrary::FindBest(uint8_t numCbvs, uint8_t numSrvs, uint8_t numSamplers, uint8_t numUavs)
	{
		ShaderResourceCounts counts{ .NumCBVs = numCbvs, .NumSamplers = numSamplers, .NumSRVs = numSrvs, .NumUAVs = numUavs };

		RootSignatureTier tier = counts.Quantize();

		if (tier >= RootSignatureTier::NUM_TIERS)
		{
			return nullptr;
		}

		return m_rootSigs[(uint32_t)tier];
	}

	bool D3D12RootSignatureLibrary::CreateRootSignature(RootSignatureTier tier)
	{
		ShaderResourceCounts counts = { };

		switch (tier)
		{
			case TIER_1CBV_1SRV_1SAMPLER:
				counts.NumCBVs = 1;
				counts.NumSRVs = 1;
				counts.NumSamplers = 1;
				counts.NumUAVs = 0;
				break;
			case TIER_2CBV_2SRV_2SAMPLER:
				counts.NumCBVs = 2;
				counts.NumSRVs = 2;
				counts.NumSamplers = 2;
				counts.NumUAVs = 0;
				break;
			case TIER_4CBV_4SRV_4SAMPLER:
				counts.NumCBVs = 4;
				counts.NumSRVs = 4;
				counts.NumSamplers = 4;
				counts.NumUAVs = 0;
				break;
			case TIER_8CBV_8SRV_8SAMPLER:
				counts.NumCBVs = 8;
				counts.NumSRVs = 8;
				counts.NumSamplers = 8;
				counts.NumUAVs = 0;
				break;
			case TIER_16CBV_16SRV_16SAMPLER:
				counts.NumCBVs = 16;
				counts.NumSRVs = 16;
				counts.NumSamplers = 16;
				counts.NumUAVs = 0;
				break;
			case TIER_1CBV_1SRV_1UAV_1SAMPLER:
				counts.NumCBVs = 1;
				counts.NumSRVs = 1;
				counts.NumSamplers = 1;
				counts.NumUAVs = 1;
				break;
			case TIER_2CBV_2SRV_2UAV_2SAMPLER:
				counts.NumCBVs = 2;
				counts.NumSRVs = 2;
				counts.NumSamplers = 2;
				counts.NumUAVs = 2;
				break;
			case TIER_4CBV_4SRV_4UAV_4SAMPLER:
				counts.NumCBVs = 4;
				counts.NumSRVs = 4;
				counts.NumSamplers = 4;
				counts.NumUAVs = 4;
				break;
			case TIER_8CBV_8SRV_8UAV_8SAMPLER:
				counts.NumCBVs = 8;
				counts.NumSRVs = 8;
				counts.NumSamplers = 8;
				counts.NumUAVs = 8;
				break;
			case TIER_16CBV_16SRV_16UAV_16SAMPLER:
				counts.NumCBVs = 16;
				counts.NumSRVs = 16;
				counts.NumSamplers = 16;
				counts.NumUAVs = 16;
				break;
		}
	
		D3D12_ROOT_SIGNATURE_DESC desc = { };
		ID3D12RootSignature* rootSig = nullptr;

		CreateRootSignatureDesc(&desc, counts.NumCBVs, counts.NumSamplers, counts.NumSRVs, counts.NumUAVs);

		ID3DBlob* serializedBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedBlob, &errorBlob)))
		{
			if (m_handler != nullptr)
			{
				m_handler->Error("Error creating root signature: %s", (const char*)errorBlob->GetBufferPointer());
				FreeRootSignatureDesc(&desc);
				return false;
			}
		}

		FreeRootSignatureDesc(&desc);

		if (FAILED(
			m_device->CreateRootSignature(
				1,
				serializedBlob->GetBufferPointer(),
				serializedBlob->GetBufferSize(),
				IID_PPV_ARGS(&rootSig)))) 
		{
			if (m_handler)
				m_handler->Error("Failed to create root signature");
			return false;
		}

		serializedBlob->Release();
		if (errorBlob != nullptr)
		{
			errorBlob->Release();
		}

		m_rootSigs[(uint32_t)tier] = rootSig;

		return true;
	}
}