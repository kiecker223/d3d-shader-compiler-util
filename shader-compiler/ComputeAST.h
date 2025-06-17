#pragma once

#include "AST.h"


class ComputeAST : public ASTBase
{
public:
	ComputeAST();
	~ComputeAST();


	bool ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool HandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool Interpret() override;

private:

	COMPUTE_PIPELINE_DESC m_Desc;

};
