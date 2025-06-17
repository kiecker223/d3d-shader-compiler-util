#pragma once

#include "AST.h"
#include <vector>
#include <memory>


class IASTNode;
class ASTInitializerList;

class GraphicsAST : public ASTBase
{
public:

	GraphicsAST();
	~GraphicsAST();

	bool ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool HandleParse(ASTParsedTokens& tokens, const ASTToken& token) override;

	bool Interpret() override;

	inline FULL_PIPELINE_DESCRIPTOR GetDesc() const 
	{
		return m_Desc;
	}

private:

	// Private to save typing time without
	// putting weird typedefs in global scope

	bool m_PipelineParsed = false;


	FULL_PIPELINE_DESCRIPTOR m_Desc;

	std::shared_ptr<ASTInitializerList> m_Initializer;
};