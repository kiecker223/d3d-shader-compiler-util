#pragma once

#include "AST.h"


class ComputeAST : public ASTBase
{
public:
	ComputeAST(COMPUTE_PIPELINE_DESC& desc);
	~ComputeAST();

	bool Interpret() override;

private:

	COMPUTE_PIPELINE_DESC& m_Desc;

};
