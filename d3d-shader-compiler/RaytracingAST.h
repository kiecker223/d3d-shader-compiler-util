#pragma once

#include "AST.h"


class RaytracingAST : public ASTBase
{
public:
	RaytracingAST();
	~RaytracingAST();

	bool Interpret() override;

private:

	RAYTRACING_PIPELINE_DESC m_Desc;
};