#pragma once

#include "Pipeline.h"


void SetCompilationMode(ShaderCompilationType Mode);

FULL_PIPELINE_DESCRIPTOR LoadGraphicsPipeline(const std::string& ShaderFile);

COMPUTE_PIPELINE_DESC LoadComputePipeline(const std::string& ShaderFile);

RAYTRACING_PIPELINE_DESC LoadRaytracingPipeline(const std::string& ShaderFile);
