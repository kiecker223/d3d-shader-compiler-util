#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Pipeline.h"


typedef enum EAST_NODE_TYPE {
	AST_NODE_TYPE_FUNCTION_DECL,
	AST_NODE_TYPE_STRUCT_DECL,
	AST_NODE_TYPE_RESOURCES_DECL,
	AST_NODE_TYPE_GFX_PIPELINE_DECL,
	AST_NODE_TYPE_RAY_PIPELINE_DECL
} EAST_NODE_TYPE;

class IASTNode
{
public:

	virtual EAST_NODE_TYPE Type() const = 0;
};

typedef std::shared_ptr<IASTNode> IASTNodePtr;

class ASTStructDecl : public IASTNode
{
public:

	struct Member
	{
		std::string Type;
		std::string Name;
		std::string Semantic;
	};
	
	ASTStructDecl();
	~ASTStructDecl();

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_STRUCT_DECL;
	}

	std::string Name;
	std::vector<Member> Members;
};

class ASTFunctionDecl : public IASTNode
{
public:

	struct Param
	{
		std::string Modifier;
		std::string Type;
		std::string Name;
		std::string Semantic;
	};

	ASTFunctionDecl();
	~ASTFunctionDecl();

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_FUNCTION_DECL;
	}

	std::string Name;
	std::string ReturnType;
	std::vector<Param> Params;
};

class ASTResourcesDecl : public IASTNode
{
public:

	ASTResourcesDecl();
	~ASTResourcesDecl();

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_RESOURCES_DECL;
	}

	std::string Reconstruct() const;

	PIPELINE_RESOURCE_COUNTERS Counts;
};

class ASTGfxPipelineDecl : public IASTNode
{
public:

	ASTGfxPipelineDecl();
	~ASTGfxPipelineDecl();

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_GFX_PIPELINE_DECL;
	}

};

class ASTRayPipelineDecl : public IASTNode
{
public:

	ASTRayPipelineDecl();
	~ASTRayPipelineDecl();

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_RAY_PIPELINE_DECL;
	}


};