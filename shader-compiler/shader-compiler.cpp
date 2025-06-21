#include "argparse.h"
#include "Utils.h"
#include "ComputeAST.h"
#include "GraphicsAST.h"
#include "RaytracingAST.h"
#include "ShaderCompiler.h"
#include "PipelineCompiler.h"


struct ShaderCompilerArgs : public argparse::Args
{
	std::string& ShaderFolder = kwarg("s,shaders", "The folder containing the shaders you wish to compile");
	std::string& ShaderModel = kwarg("m,model", "The shader model you want to compile your shaders to. Uses direct3d shaders models. Eg. 6_5");

	std::string& D3DExtraFlags = kwarg("d3d,d3d-extra", "Extra flags to compile your DirectX shaders with").set_default("");
	std::string& VKExtraFlags = kwarg("vk,vk-extra", "Extra flags to compile your Vulkan shaders with").set_default("");
	
	std::string& D3DDebugOverride = kwarg("d3ddo,d3d-debug-override", "Completely override flags that DirectX shaders compile with in debug mode. Default flags: \"-Zi\"").set_default("");
	std::string& VKDebugOverride = kwarg("vkdo,vk-debug-override", "Completely override flags that Vulkan shaders compile with in debug mode. Default flags: \"-Zi -spirv\"").set_default("");

	std::string& D3DReleaseOverride = kwarg("d3dro,d3d-release-override", "Completely override flags that DirectX shaders compile with in release mode. Default flags: \"-O3\"").set_default("");
	std::string& VKReleaseOverride = kwarg("vkro,vk-release-override", "Completely override flags that Vulkan shaders compile with in release mode. Default flags: \"-O3 -spirv\"").set_default("");
};

int main(int argc, char** argv)
{
	auto args = argparse::parse<ShaderCompilerArgs>(argc, argv);

	{
		bool hasError = false;
		if (args.ShaderFolder == "")
		{
			std::cout << "[ERROR] -s,--shaders MUST be set" << std::endl;
			hasError = true;
		}
		if (args.ShaderModel == "")
		{
			std::cout << "[ERROR] -m,--model MUST be set" << std::endl;
			hasError = true;
		}

		if (hasError)
		{
			exit(1);
		}
	}

	ShaderCompiler compiler(std::filesystem::path(args.ShaderFolder));

	if (!compiler.InitializeDxcResources())
	{
		std::cout << "[ERROR] Failed to initalize dxc resources" << std::endl;
		exit(1);
	}

	if (!compiler.SetShaderModel(args.ShaderModel))
	{
		std::cout << "[ERROR] -m,--model is in the incorrect format. Expected <num>_<num>" << std::endl;
		exit(1);
	}

	if (!args.D3DExtraFlags.empty())
	{
		compiler.SetD3DExtraFlags(args.D3DExtraFlags);
	}
	if (!args.VKExtraFlags.empty())
	{
		compiler.SetVulkanExtraFlags(args.VKExtraFlags);
	}

	if (!args.D3DDebugOverride.empty())
	{
		compiler.SetD3DOverrideFlags(WithDebugInfo_NoOptimization, args.D3DDebugOverride);
	}
	if (!args.D3DReleaseOverride.empty())
	{
		compiler.SetD3DOverrideFlags(WithoutDebugInfo_Optimize, args.D3DReleaseOverride);
	}

	if (!args.VKDebugOverride.empty())
	{
		compiler.SetVulkanOverrideFlags(WithDebugInfo_NoOptimization, args.VKDebugOverride);
	}
	if (!args.VKReleaseOverride.empty())
	{
		compiler.SetVulkanOverrideFlags(WithoutDebugInfo_Optimize, args.VKReleaseOverride);
	}

	if (!compiler.SetupArgs())
	{
		std::cout << "[ERROR] Failed to setup arguments for the compiler" << std::endl;
		exit(1);
	}

	PipelineCompiler pipelineCompiler(&compiler);

	pipelineCompiler.SetSrcDir(args.ShaderFolder);
	pipelineCompiler.Load();
}