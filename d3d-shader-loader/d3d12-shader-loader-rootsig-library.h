#pragma once
#include <stdint.h>
#include <d3d12.h>
#include "d3d-shader-loader-helper.h"


namespace LoaderPriv {

	enum RootSignatureTier 
	{
		TIER_1CBV_1SRV_1SAMPLER,
		TIER_2CBV_2SRV_2SAMPLER,
		TIER_4CBV_4SRV_4SAMPLER,
		TIER_8CBV_8SRV_8SAMPLER,
		TIER_16CBV_16SRV_16SAMPLER,
		TIER_1CBV_1SRV_1UAV_1SAMPLER,
		TIER_2CBV_2SRV_2UAV_2SAMPLER,
		TIER_4CBV_4SRV_4UAV_4SAMPLER,
		TIER_8CBV_8SRV_8UAV_8SAMPLER,
		TIER_16CBV_16SRV_16UAV_16SAMPLER,
		NUM_TIERS,
		INVALID_TIER = 0xffffffff
	};

	struct ShaderResourceCounts
	{
		uint8_t NumCBVs;
		uint8_t NumSRVs;
		uint8_t NumSamplers;
		uint8_t NumUAVs;

		/*
		* @brief: Take the given resource counts, and turn them to
		* its relevant shader tier. We don't actually need to build
		* a unique root signature for each shader, we can sortof
		* make a 1 size fits all solution
		*/
		RootSignatureTier Quantize() const;
	};

	class D3D12RootSignatureLibrary
	{
	public:

		/*
		* @brief: Build the D3D12RootSignatureLibrary. Keeps a pointer to the ID3D12Device, but does not call Release
		* Takes soft ownership
		*/
		explicit D3D12RootSignatureLibrary(ID3D12Device* device);
		~D3D12RootSignatureLibrary();

		/*
		* @brief: Sets the print handler. Used for user logging
		*/
		void SetPrintHandler(ID3DShaderLoaderPrintHandler* handler);

		/*
		* @brief: Builds the associated ID3D12RootSignature objects
		* @returns: Returns false if any root signatures failed to build.
		*/
		bool BuildRootSignatures();

		/*
		* @brief: Finds the best ID3D12RootSignature based on the resource counts passed.
		* @returns: returns a valid ID3D12RootSignature object if its able to find one big enough. Otherwise returns null.
		* Maintains ownership of the ID3D12RootSignature objects. So do not call "Release()".
		*/
		ID3D12RootSignature* FindBest(uint8_t numCbvs, uint8_t numSrvs, uint8_t numSamplers, uint8_t numUavs);

	private:

		bool CreateRootSignature(RootSignatureTier tier);

		ID3D12Device* m_device;
		ID3D12RootSignature* m_rootSigs[RootSignatureTier::NUM_TIERS];

		ID3DShaderLoaderPrintHandler* m_handler;
	};

}

