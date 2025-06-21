#include "ComputeAST.h"


ComputeAST::ComputeAST(COMPUTE_PIPELINE_DESC& desc) : 
	ASTBase(),
	m_Desc(desc)
{
}

ComputeAST::~ComputeAST()
{
}

bool ComputeAST::Interpret()
{
	// Nothing to do.
	return true;
}
