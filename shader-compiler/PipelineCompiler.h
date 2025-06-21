#pragma once

#include "Pipeline.h"
#include "AST.h"
#include "ShaderCompiler.h"
#include "nlohmann.hpp"
#include <memory>
#include <filesystem>


// This would be the place where 
// you would setup encryption when I
// add signing and encryption to this project
class PipelineCompiler
{
public:

	PipelineCompiler() = delete;
	PipelineCompiler(
		ShaderCompiler* compiler
	);

	void SetSrcDir(const std::filesystem::path& path);

	bool Load();

	void WriteToFile(const std::filesystem::path& dstFile);

private:

	bool LoadGfxFile(const std::filesystem::path& path);

	bool LoadCmptFile(const std::filesystem::path& path);

	bool LoadRayFile(const std::filesystem::path& path);

	bool LoadFileImpl(const std::filesystem::path& path, ASTBase* ast, const std::string& type);

	std::string CutPipelineBlock(std::string& fileData, ASTBase* ast);

	std::filesystem::path m_SrcPath;

	nlohmann::json m_Json;

	// Not a shared pointer because 
	// we don't own this, m_Compiler
	// is on the stack in main()
	ShaderCompiler* m_Compiler;

};