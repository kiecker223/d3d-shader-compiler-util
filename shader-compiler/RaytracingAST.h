#pragma once

#include "AST.h"


class RaytracingAST : public ASTBase
{
public:
	RaytracingAST(RAYTRACING_PIPELINE_DESC& desc);
	~RaytracingAST();

	bool Interpret() override;

	inline const RAYTRACING_PIPELINE_DESC& GetDesc() const
	{
		return m_Desc;
	}

private:

	RAYTRACING_PIPELINE_DESC& m_Desc;
};