#include "Pipeline.h"
#include <iostream>
#include "Utils.h"



GFX_RASTER_DESC CreateDefaultGFXRasterDesc()
{
	GFX_RASTER_DESC Result = { };
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


FULL_PIPELINE_DESCRIPTOR CreateDefaultDescriptor()
{
	FULL_PIPELINE_DESCRIPTOR Result = { };
	Result.PolygonType = POLYGON_TYPE_TRIANGLES;
	Result.RasterDesc = CreateDefaultGFXRasterDesc();
	Result.bEnableAlphaToCoverage = false;
	Result.bIndependentBlendEnable = false;
	Result.NumRenderTargets = 1;
	for (uint32_t i = 0; i < 8; i++)
	{
		Result.RtvDescs[i] = CreateDefaultGFXRenderTargetDesc();
	}
	return Result;
}
