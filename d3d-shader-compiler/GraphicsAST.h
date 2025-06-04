#pragma once

#include "AST.h"
#include <unordered_map>


class GraphicsAST : public ASTBase
{
public:

	GraphicsAST();
	~GraphicsAST();

	bool Interpret() override;

	inline FULL_PIPELINE_DESCRIPTOR GetDesc() const 
	{
		return m_Desc;
	}

private:


	FULL_PIPELINE_DESCRIPTOR m_Desc;

};