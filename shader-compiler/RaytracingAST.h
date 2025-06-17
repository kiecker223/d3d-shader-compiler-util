#pragma once

#include "AST.h"


class RaytracingAST : public ASTBase
{
public:
	RaytracingAST();
	~RaytracingAST();

	bool ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool HandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool Interpret() override;

private:

	RAYTRACING_PIPELINE_DESC m_Desc;

	bool m_PipelineParsed;

	std::shared_ptr<ASTInitializerList> m_Initializer;
};