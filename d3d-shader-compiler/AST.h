#pragma once

#include "Utils.h"
#include <memory>
#include "Pipeline.h"


class IASTPrintHandler
{
public:

	virtual void ErrorImpl(const char* message) = 0;
	virtual void WarnImpl(const char* message) = 0;
	virtual void MessageImpl(const char* message) = 0;

	void Error(const char* fmt, ...);
	void Warn(const char* fmt, ...);
	void Message(const char* fmt, ...);
};


typedef enum EAST_TOKEN_TYPE {
	AST_TOKEN_TYPE_NONE = 0,

	// General syntax stuff
	AST_TOKEN_TYPE_LEFT_CURLY,
	AST_TOKEN_TYPE_RIGHT_CURLY,
	AST_TOKEN_TYPE_LEFT_PARENTHESIS,
	AST_TOKEN_TYPE_RIGHT_PARANTHESIS,
	AST_TOKEN_TYPE_LEFT_GATOR,
	AST_TOKEN_TYPE_RIGHT_GATOR,
	AST_TOKEN_TYPE_COMMA,
	AST_TOKEN_TYPE_COLON,
	AST_TOKEN_TYPE_EQUALS,
	AST_TOKEN_TYPE_SEMICOLON,
	AST_TOKEN_TYPE_LEFT_SQUARE,
	AST_TOKEN_TYPE_RIGHT_SQUARE,
	AST_TOKEN_TYPE_SINGLE_QUOTE,
	AST_TOKEN_TYPE_DOUBLE_QUOTE,

	// Catch all for math stuff, we don't care about this
	AST_TOKEN_TYPE_MATH_OPERATION,

	// Different keyword types
	AST_TOKEN_TYPE_PARAM_MODIFIER,
	AST_TOKEN_TYPE_BUILTIN_DATATYPE,
	AST_TOKEN_TYPE_STRUCT_KEYWORD,
	AST_TOKEN_TYPE_HLSL_KEYWORD,

	// Catch all for when we think we're looking at an identifier
	AST_TOKEN_TYPE_GENERAL_IDENTIFIER,

	// Special marker that this node needs a second pass
	AST_TOKEN_TYPE_UNKNOWN
} EAST_TOKEN_TYPE;


typedef struct TOKEN {
	std::string Data;
	uint32_t Line;
} TOKEN;

class ASTToken
{
public:
	TOKEN Data;
	EAST_TOKEN_TYPE Type;

	inline std::string GetData() const
	{
		return Data.Data;
	}
};

template<typename T>
class ASTItemsList
{
public:

	ASTItemsList() = delete;

	ASTItemsList(std::vector<T>& inItems) :
		Items(Items),
		Ptr(0)
	{
	}

	ASTItemsList(const ASTItemsList<T>& other) :
		Items(other.Items),
		Ptr(other.Ptr)
	{ }

	inline bool Advance()
	{
		if (Ptr + 1 > Items.size() - 1)
		{
			return false;
		}

		Ptr++;
		return true;
	}

	inline size_t Count() const
	{
		return Items.size();
	}

	inline const T& Current() const
	{
		return Items[Ptr];
	}

	size_t Ptr;
	const std::vector<T>& Items;
};

typedef ASTItemsList<TOKEN> ASTUnparsedTokens;
typedef ASTItemsList<ASTToken> ASTParsedTokens;

class ASTBase
{
public:

	ASTBase() : m_Print(nullptr), m_Counts{0, 0, 0, 0} {}

	bool LoadFile(const std::filesystem::path& path);

	bool Parse(ASTUnparsedTokens& tokens);

	bool SecondPassParse(ASTParsedTokens& tokens);

	virtual bool ShouldHandleParse(ASTParsedTokens& tokens, const ASTToken& token) = 0;

	virtual bool HandleParse(ASTParsedTokens& tokens, const ASTToken& token) = 0;

	void SetPrintHandler(IASTPrintHandler* handler);

protected:

	bool ParseResourcesBlock(ASTParsedTokens& tokens);

	bool ParseStructDefinition(ASTParsedTokens& tokens);

	bool ParseFunctionDefinition(ASTParsedTokens& tokens);

	bool IsFunctionDeclaration(ASTParsedTokens tokens);

	bool IsStructDefined(const std::string& name);

	bool IsValidType(const std::string& type) const;

	bool IsSystemType(const std::string& type) const;

	bool IsHLSLReservedWord(const std::string& word) const;

	std::vector<ASTToken> BreakUpStringWithSyntaxSugar(const std::string& statement, uint32_t lineNumber) const;

	bool IsValidParamModifier(const std::string& modifier) const;

	std::vector<std::string> m_Structs;
	std::map<std::string, ASTToken> m_StructsParsed;

	std::vector<std::string> m_Funcs;
	std::map<std::string, ASTToken> m_FuncsParsed;

	bool m_UnrecoverableError = false;

	PIPELINE_RESOURCE_COUNTERS m_Counts;

	std::vector<ASTToken> m_Tokens;
	IASTPrintHandler* m_Print;
};


