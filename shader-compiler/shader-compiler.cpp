#include "argparse.h"
#include "Utils.h"
#include "ComputeAST.h"
#include "GraphicsAST.h"
#include "RaytracingAST.h"


struct ShaderCompilerArgs : public argparse::Args
{
	std::string& ShaderFolder = kwarg("s,shaders", "The folder containing the shaders you wish to compile");
	std::string& DstFolder = kwarg("d,destination", "The destination folder you wish to put compiled shaders");
	std::string& ShaderModel = kwarg("m,model", "The shader model you want to compile your shaders to. Uses direct3d shaders models. Eg. 6_5");
};

int main(int argc, char** argv)
{
	auto args = argparse::parse<ShaderCompilerArgs>(argc, argv);


}