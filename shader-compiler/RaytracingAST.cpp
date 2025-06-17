#include "RaytracingAST.h"


RaytracingAST::RaytracingAST() : ASTBase()
{
}

RaytracingAST::~RaytracingAST()
{
}

bool RaytracingAST::ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	if (m_PipelineParsed)
	{
		return false;
	}

	if (token.GetData() != "Pipeline")
	{
		return false;
	}

	{
		ASTParsedTokens toks = tokens;
		if (!toks.Advance())
		{
			m_Print->Error("Unexpected end of file on line %d", toks.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		if (toks.Current().Type != AST_TOKEN_TYPE_EQUALS)
		{
			m_Print->Error("Expected assignment to \"Pipeline\" variable, got \"%s\" on line %d",
				toks.Current().GetDataPtr(),
				toks.Current().GetLine()
			);
			return false;
		}

		if (!toks.Advance())
		{
			m_Print->Error("Unexpected end of file on line %d", toks.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		if (toks.Current().Type != AST_TOKEN_TYPE_LEFT_CURLY)
		{
			m_Print->Error("Unexpected syntax. \"Pipeline = %s\", expected \"Pipeline = {\" on line %d",
				toks.Current().GetDataPtr(),
				toks.Current().GetLine()
			);
		}
	}

	return true;
}

bool RaytracingAST::HandleParse(ASTParsedTokens& tokens, const ASTToken& token)
{
	if (m_PipelineParsed)
	{
		return false;
	}

	// Because toks was able to get here, we can assume 
	// these will succeed
	tokens.Advance(); // tokens.Current() = '='
	tokens.Advance(); // tokens.Current() = '{'

	m_Initializer = std::make_shared<ASTInitializerList>();
	NameList emptyName;

	if (!ParseInitializerScope(tokens, emptyName, m_Initializer))
	{
		m_Print->Error("Failed to parse Pipeline block");
		return false;
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_SEMICOLON)
	{
		m_Print->Error("Syntax error. Expected ';' on line %d. Got '%s'",
			tokens.Current().GetLine(),
			tokens.Current().GetDataPtr());
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	m_PipelineParsed = true;
	return true;
}

bool RaytracingAST::Interpret()
{
	return false;
}
