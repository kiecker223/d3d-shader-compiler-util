#pragma once

#include "AST.h"
#include <vector>
#include <memory>


class IASTNode;
class ASTInitializerList;

class GraphicsAST : public ASTBase
{
public:
	GraphicsAST() = delete;
	GraphicsAST(FULL_PIPELINE_DESCRIPTOR& desc);
	~GraphicsAST();

	bool Interpret() override;

	inline const FULL_PIPELINE_DESCRIPTOR& GetDesc() const 
	{
		return m_Desc;
	}

private:

	FULL_PIPELINE_DESCRIPTOR& m_Desc;
};