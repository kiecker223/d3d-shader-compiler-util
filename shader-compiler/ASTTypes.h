#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Pipeline.h"


typedef enum EAST_NODE_TYPE {
	AST_NODE_TYPE_FUNCTION_DECL,
	AST_NODE_TYPE_STRUCT_DECL,
	AST_NODE_TYPE_RESOURCES_DECL,
	AST_NODE_TYPE_GFX_PIPELINE_DECL,
	AST_NODE_TYPE_RAY_PIPELINE_DECL,
	AST_NODE_TYPE_ASSIGNMENT,
	AST_NODE_TYPE_INITIALIZER_LIST,
	AST_NODE_TYPE_VALUE
} EAST_NODE_TYPE;


class ASTAssignment;
class ASTInitializerList;

class IASTNode
{
public:

	virtual EAST_NODE_TYPE Type() const = 0;

};

class ASTInitializerList : public IASTNode
{
public:

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_INITIALIZER_LIST;
	}

	std::vector<
		std::shared_ptr<
		IASTNode
		>
	> Assignments;
};

class ASTAssignmentValue : public IASTNode
{
public:

	ASTAssignmentValue() = default;
	ASTAssignmentValue(const std::string& value) :
		Value(value)
	{
	}

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_VALUE;
	}

	std::string Value;
};

class ASTAssignment : public IASTNode
{
public:

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_ASSIGNMENT;
	}

	uint32_t NumNames() const
	{
		return (uint32_t)Names.size();
	}

	std::string GetName() const
	{
		return Names[0];
	}

	// Support for multiple name assignment
	std::vector<std::string> Names;
	std::shared_ptr<IASTNode> Value;
};

class ASTStructDecl : public IASTNode
{
public:

	struct Member
	{
		std::string Modifier;
		std::string Type;
		std::string Name;
		std::string Semantic;
	};
	
	ASTStructDecl() {}
	~ASTStructDecl() {}

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_STRUCT_DECL;
	}

	std::string Name;
	std::map<std::string, Member> Members;
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

	ASTFunctionDecl() {}
	~ASTFunctionDecl() {}

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_FUNCTION_DECL;
	}

	std::string Name;
	std::string ReturnType;
	std::string ReturnSemantic;
	std::vector<Param> Params;
};

class ASTResourcesDecl : public IASTNode
{
public:

	ASTResourcesDecl() { }
	~ASTResourcesDecl() { }

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_RESOURCES_DECL;
	}

	PIPELINE_RESOURCE_COUNTERS Counts = { 0 };
};

class ASTGfxPipelineDecl : public IASTNode
{
public:

	ASTGfxPipelineDecl() {}
	~ASTGfxPipelineDecl() {}

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_GFX_PIPELINE_DECL;
	}

};

class ASTRayPipelineDecl : public IASTNode
{
public:

	ASTRayPipelineDecl() {}
	~ASTRayPipelineDecl() {}

	EAST_NODE_TYPE Type() const override
	{
		return AST_NODE_TYPE_RAY_PIPELINE_DECL;
	}


};