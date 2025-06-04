#include "GraphicsAST.h"

GraphicsAST::GraphicsAST() : ASTBase()
{
}

GraphicsAST::~GraphicsAST()
{
}

bool GraphicsAST::ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	return false;
}

bool GraphicsAST::HandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	return false;
}

bool GraphicsAST::InterpretImpl()
{
	return false;
}

