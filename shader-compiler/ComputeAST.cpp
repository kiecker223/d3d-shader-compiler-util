#include "ComputeAST.h"


ComputeAST::ComputeAST() : ASTBase()
{
}

ComputeAST::~ComputeAST()
{
}

bool ComputeAST::ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	return false;
}

bool ComputeAST::HandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	return false;
}

bool ComputeAST::Interpret()
{
	if (!m_ResourcesBlockParsed)
	{
		return false;
	}

	m_Desc.Counts = m_Counts;
}
