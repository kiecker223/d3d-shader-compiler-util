#pragma once

#include "Utils.h"
#include <memory>
#include "Pipeline.h"
#include "ASTTypes.h"





typedef enum EAST_TOKEN_TYPE {
	AST_TOKEN_TYPE_NONE = 0,

	// General syntax stuff
	AST_TOKEN_TYPE_LEFT_CURLY,
	AST_TOKEN_TYPE_RIGHT_CURLY,
	AST_TOKEN_TYPE_LEFT_PARENTHESIS,
	AST_TOKEN_TYPE_RIGHT_PARENTHESIS,
	AST_TOKEN_TYPE_LEFT_GATOR,
	AST_TOKEN_TYPE_RIGHT_GATOR,
	AST_TOKEN_TYPE_COMMA,
	AST_TOKEN_TYPE_COLON,
	AST_TOKEN_TYPE_DOUBLE_COLON,
	AST_TOKEN_TYPE_EQUALS,
	AST_TOKEN_TYPE_SEMICOLON,
	AST_TOKEN_TYPE_LEFT_SQUARE,
	AST_TOKEN_TYPE_RIGHT_SQUARE,
	AST_TOKEN_TYPE_DOUBLE_LEFT_SQUARE,
	AST_TOKEN_TYPE_DOUBLE_RIGHT_SQUARE,
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

#define SCOPE_UNDERFLOW_CHECK 4096

typedef struct TOKEN {
	std::string Data;
	uint32_t Line;
} TOKEN;

class ASTToken
{
public:
	TOKEN Data;
	EAST_TOKEN_TYPE Type;

	inline const std::string& GetData() const
	{
		return Data.Data;
	}

	inline const char* GetDataPtr() const
	{
		return Data.Data.c_str();
	}

	inline uint32_t GetLine() const
	{
		return Data.Line;
	}
};

template<typename T>
class ASTItemsList
{
public:

	ASTItemsList() = delete;

	ASTItemsList(std::vector<T>& inItems) :
		Items(inItems),
		Ptr(0)
	{
	}

	ASTItemsList(const ASTItemsList<T>& other) :
		Items(other.Items),
		Ptr(other.Ptr)
	{ }

	inline bool Advance()
	{
		if (!CanPeekNext())
		{
			return false;
		}

		Ptr++;
		return true;
	}

	inline bool CanPeekNext() const
	{
		if (Ptr + 1 > Items.size() - 1)
		{
			return false;
		}
		return true;
	}

	inline const T& PeekNext() const
	{
		return Items[Ptr + 1];
	}

	inline size_t Count() const
	{
		return Items.size();
	}

	inline const T& Current() const
	{
		return Items[Ptr];
	}

	inline const T& Last() const
	{
		if (Ptr == 0)
		{
			return Current();
		}

		return Items[Ptr - 1];
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

	virtual bool Interpret() = 0;

	void SetPrintHandler(IPrintHandler* handler);

	inline const std::string& GetReconstructedResourcesBlock() const
	{
		return m_ResourcesBlockStr;
	}

	inline const PIPELINE_RESOURCE_COUNTERS& GetCounts() const
	{
		return m_Counts;
	}

	inline void GetPipelineBlockLines(uint32_t& outStart, uint32_t& outEnd)
	{
		outStart = m_PipelineBlockStart;
		outEnd = m_PipelineBlockEnd;
	}

	inline bool GetFuncDecl(const std::string& funcName, ASTFunctionDecl& outFunc)
	{
		auto findRes = m_FuncsParsed.find(funcName);
		if (findRes == m_FuncsParsed.end())
		{
			return false;
		}

		outFunc = findRes->second;
		return true;
	}

	inline bool GetStructDecl(const std::string& structName, ASTStructDecl& outStruct)
	{
		auto findRes = m_StructsParsed.find(structName);
		if (findRes == m_StructsParsed.end())
		{
			return false;
		}

		outStruct = findRes->second;
		return true;
	}


protected:

	typedef std::vector<std::string> NameList;

	bool ParseResourcesBlock(ASTParsedTokens& tokens);

	bool IsPipelineStatement(ASTParsedTokens& tokens);

	bool ParsePipelineStatement(ASTParsedTokens& tokens);

	bool ParseStructDefinition(ASTParsedTokens& tokens);

	bool ParseInitializerScope(
		ASTParsedTokens& tokens,
		const NameList& name,
		std::shared_ptr<ASTInitializerList> outList
	);

	bool ParseRegisterStatement(ASTParsedTokens& tokens);

	bool ParseFunctionDefinition(ASTParsedTokens& tokens);

	bool IsFunctionDeclaration(ASTParsedTokens tokens);

	bool IsStructDefined(const std::string& name);

	bool IsValidType(const std::string& type) const;

	bool HasParsedFunction(const std::string& name) const;

	bool IsSystemType(const std::string& type) const;

	bool IsHLSLReservedWord(const std::string& word) const;

	/*
	* @summary: Given the current scope, try to advance to the end of the given scope
	* delimited by '{' and '}' respectively. This makes an assumption that tokens.Current() will be valid
	* and is either inside of, or going to descend you into the scope you wish to check
	* @param tokens: The tokens object to advance
	* @param scopeCt: If you're currently in a scope, you're essentially setting this
	* to the number of times you want to exit a scope.
	* So if 
	* { SomeToken <- you're here SomeOtherToken } <- And you want to be here
	* You would set scopeCt to 1
	*/
	bool AdvanceToEndOfScope(ASTParsedTokens& tokens, uint32_t scopeCt = 0);

	void AdvanceToEndOfStatement(ASTParsedTokens& tokens);

	std::vector<ASTToken> BreakUpStringWithSyntaxSugar(const std::string& statement, uint32_t lineNumber) const;

	bool IsValidParamModifier(const std::string& modifier) const;

	std::vector<std::string> m_Structs;
	std::map<std::string, ASTStructDecl> m_StructsParsed;

	std::vector<std::string> m_Funcs;
	std::map<std::string, ASTFunctionDecl> m_FuncsParsed;

	bool m_UnrecoverableError = false;

	PIPELINE_RESOURCE_COUNTERS m_Counts;
	std::string m_ResourcesBlockStr;

	uint32_t m_PipelineBlockStart = 0;
	uint32_t m_PipelineBlockEnd = 0;

	std::vector<ASTToken> m_Tokens;
	IPrintHandler* m_Print;

	bool m_ResourcesBlockParsed = false;

	bool m_PipelineParsed = false;
	std::shared_ptr<ASTInitializerList> m_PipelineNode;
};


