#include "GraphicsAST.h"
#include <unordered_map>
#include <memory>



GraphicsAST::GraphicsAST(FULL_PIPELINE_DESCRIPTOR& desc) : 
	ASTBase(),
	m_Desc(desc)
{
}

GraphicsAST::~GraphicsAST()
{
}

bool GraphicsAST::Interpret()
{
	return false;
}


