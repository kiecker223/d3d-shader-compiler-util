#include "RaytracingAST.h"


RaytracingAST::RaytracingAST(RAYTRACING_PIPELINE_DESC& desc) : 
	ASTBase(),
	m_Desc(desc)
{
}

RaytracingAST::~RaytracingAST()
{
}

bool RaytracingAST::Interpret()
{
	return false;
}
