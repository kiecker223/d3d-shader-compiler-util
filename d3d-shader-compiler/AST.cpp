#include "AST.h"
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <iostream>
#include <regex>


class ASTDefaultPrintHandler : public IASTPrintHandler
{
public:

	void ErrorImpl(const char* message) override
	{
		std::cout << "[ERROR] " << message << std::endl;
	}

	void WarnImpl(const char* message) override
	{
		std::cout << "[WARN] " << message << std::endl;
	}

	void MessageImpl(const char* message) override
	{
		std::cout << "[MSG] " << message << std::endl;
	}
};

static ASTDefaultPrintHandler g_DefaultPrint;

void IASTPrintHandler::Error(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->ErrorImpl(buffer);
}

void IASTPrintHandler::Warn(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->WarnImpl(buffer);
}

void IASTPrintHandler::Message(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->MessageImpl(buffer);
}

std::vector<TOKEN> ExtractTokens(const std::string& fileData)
{
	std::vector<TOKEN> Result;
	std::istringstream Str(fileData);

	std::vector<std::string> Result;

	uint32_t i = 0;
	std::string Buff;
	while (std::getline(Str, Buff, '\n'))
	{
		i++;

		std::string currentTok;

		for (char ch : Buff)
		{
			if (IsWhiteSpace(ch))
			{
				if (currentTok != "")
				{
					TOKEN tk;
					tk.Line = i;
					tk.Data = currentTok;
					Result.push_back(tk);

					currentTok = "";
				}
			}
			else
			{
				currentTok.append(1, ch);
			}
		}

		if (currentTok != "")
		{
			TOKEN tk;
			tk.Line = i;
			tk.Data = currentTok;
			Result.push_back(tk);
		}
	}

	return Result;
}

bool ASTBase::LoadFile(const std::filesystem::path& path)
{
	if (m_Print == nullptr)
	{
		SetPrintHandler(&g_DefaultPrint);
	}

	std::wstring native = path.native();
	std::string fullPath(native.begin(), native.end());

	std::string fileData = ReadEntireFile(fullPath);
	if (fileData.size() == 0)
	{
		return false;
	}

	std::string hlslNoComments = RemoveHLSLComments(fileData);

	std::vector<TOKEN> lines = ExtractTokens(hlslNoComments);
	ASTUnparsedTokens toks(lines);
	Parse(toks);
}

bool ASTBase::Parse(ASTUnparsedTokens& tokens)
{
	m_Tokens.clear();
	while (tokens.Advance())
	{
		const TOKEN& current = tokens.Current();

		std::vector<ASTToken> expandedToks = BreakUpStringWithSyntaxSugar(current.Data, current.Line);

		for (uint32_t i = 0; i < expandedToks.size(); i++)
		{
			ASTToken& et = expandedToks[i];
			if (et.Type == AST_TOKEN_TYPE_UNKNOWN)
			{
				if (et.Data.Data == "struct")
				{
					et.Type = AST_TOKEN_TYPE_STRUCT_KEYWORD;
				}
				else if (IsSystemType(et.Data.Data))
				{
					et.Type = AST_TOKEN_TYPE_BUILTIN_DATATYPE;
				}
				else if (IsValidParamModifier(et.Data.Data))
				{
					et.Type = AST_TOKEN_TYPE_PARAM_MODIFIER;
				}
				else if (IsHLSLReservedWord(et.Data.Data))
				{
					et.Type = AST_TOKEN_TYPE_HLSL_KEYWORD;
				}
				else
				{
					et.Type = AST_TOKEN_TYPE_GENERAL_IDENTIFIER;
				}
			}
			m_Tokens.push_back(et);
		}
	}

	ASTParsedTokens newTokens(m_Tokens);

	return SecondPassParse(newTokens);
}

bool ASTBase::SecondPassParse(ASTParsedTokens& tokens)
{
	auto advanceToEndOfStatement = [&]()
		{
			while (tokens.Advance())
			{
				if (tokens.Current().Type == AST_TOKEN_TYPE_SEMICOLON)
				{
					tokens.Advance();
					break;
				}
			}
		};
	do
	{
		const ASTToken& t = tokens.Current();

		switch (t.Type)
		{
		case AST_TOKEN_TYPE_GENERAL_IDENTIFIER:
		{
			if (ShouldHandleParse(tokens, t))
			{
				if (!HandleParse(tokens, t))
				{
					if (m_UnrecoverableError)
					{
						return false;
					}
				}
			}
			if (t.GetData() == "Resources")
			{
				if (!ParseResourcesBlock(tokens))
				{
					if (m_UnrecoverableError)
					{
						return false;
					}
				}
			}

			if (IsFunctionDeclaration(tokens))
			{
				if (!ParseFunctionDefinition(tokens))
				{
					if (m_UnrecoverableError)
					{
						return false;
					}
				}
			}
			
		} break;
		case AST_TOKEN_TYPE_BUILTIN_DATATYPE:
		{
			if (IsFunctionDeclaration(tokens))
			{
				if (!ParseFunctionDefinition(tokens))
				{
					if (m_UnrecoverableError)
					{
						return false;
					}
				}
			}

			// Likely a variable declaration
			advanceToEndOfStatement();
		} break;
		case AST_TOKEN_TYPE_STRUCT_KEYWORD:
		{
			if (!ParseStructDefinition(tokens))
			{
				if (m_UnrecoverableError)
				{
					return false;
				}
			}
		} break;
		case AST_TOKEN_TYPE_HLSL_KEYWORD:
		{
			std::string keyword = t.GetData();
			if (keyword == "groupshared" ||
				keyword == "uniform" ||
				keyword == "const")
			{
				// This is likely a variable declaration, advance
				// until the first semicolon
				advanceToEndOfStatement();
			}
		} break;
		}


	} while (tokens.Advance());
}

bool ASTBase::IsValidType(const std::string& type) const
{
	if (IsSystemType(type))
	{
		return true;
	}

	for (auto& regType : m_Structs)
	{
		if (regType == type)
		{
			return true;
		}
	}

	return false;
}

bool ASTBase::IsSystemType(const std::string& type) const
{
	if (type == "void")
	{
		return true;
	}

	static std::regex r("^(int|uint|dword|half|double|float)[1-4]x[1-4]$[1-4]x[1-4]$", std::regex_constants::ECMAScript);
	if (std::regex_search(type, r))
	{
		return true;
	}

	return false;
}

bool ASTBase::IsHLSLReservedWord(const std::string& word) const
{
	static std::string keywords[] = { "AppendStructuredBuffer", "asm", "asm_fragmentBlendState", "bool", "break", "Buffer", "ByteAddressBuffercase", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment", "CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBufferdefault", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dwordelse", "export", "externfalse", "float", "for", "fxgroupGeometryShader", "groupsharedhalf", "Hullshaderif", "in", "inline", "inout", "InputPatch", "int", "interfaceline", "lineadj", "linear", "LineStreammatrix", "min16float", "min10float", "min16int", "min12int", "min16uintnamespace", "nointerpolation", "noperspective", "NULLout", "OutputPatchpackoffset", "pass", "pixelfragment", "PixelShader", "point", "PointStream", "preciseRasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3Dsample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state", "static", "string", "struct", "switch", "StructuredBuffertbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS", "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStreamuint", "uniform", "unorm", "unsignedvector", "vertexfragment", "VertexShader", "void", "volatilewhile" };

	for (uint32_t i = 0; i < (sizeof(keywords) / sizeof(std::string)); i++)
	{
		if (word == keywords[i])
		{
			return true;
		}
	}

	return false;
}

std::vector<ASTToken> ASTBase::BreakUpStringWithSyntaxSugar(const std::string& statement, uint32_t lineNumber) const
{
	size_t i = 0;
	std::string token;

	std::vector<ASTToken> result;
	ASTToken n;

#define ADD_TOKEN_AND_CLEAR do { \
	if (token.size() == 0) { break; } \
	ASTToken tmpNode; \
	tmpNode.Data.Data = token; \
	tmpNode.Data.Line = lineNumber; \
	tmpNode.Type = AST_TOKEN_TYPE_UNKNOWN; \
	result.push_back(tmpNode); \
	token = ""; \
	} while(0);

	for (i = 0; i < statement.size(); i++)
	{
		char ch = statement[i];
		switch (ch)
		{
		case '(':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "(";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_LEFT_PARENTHESIS;
			result.push_back(n);
			break;
		case ')':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = ")";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_RIGHT_PARANTHESIS;
			result.push_back(n);
			break;
		case '{':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "{";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_LEFT_CURLY;
			result.push_back(n);
			break;
		case '}':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "}";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_RIGHT_CURLY;
			result.push_back(n);
			break;
		case '<':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "<";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_LEFT_GATOR;
			result.push_back(n);
			break;
		case '>':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = ">";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_RIGHT_GATOR;
			result.push_back(n);
			break;
		case '[':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "[";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_LEFT_SQUARE;
			result.push_back(n);
			break;
		case ']':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "]";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_RIGHT_SQUARE;
			result.push_back(n);
			break;
		case '"':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "\"";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_DOUBLE_QUOTE;
			result.push_back(n);
			break;
		case '\'':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "'";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_SINGLE_QUOTE;
			result.push_back(n);
			break;
		case ':':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = ":";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_COLON;
			result.push_back(n);
			break;
		case ';':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = ";";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_SEMICOLON;
			result.push_back(n);
			break;
		case ',':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = ",";
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_COMMA;
			result.push_back(n);
			break;
		case '*':
		case '+':
		case '-':
		case '/':
		case '%':
		case '&':
		case '!':
			ADD_TOKEN_AND_CLEAR;
			n.Data.Data = "";
			n.Data.Data += ch;
			n.Data.Line = lineNumber;
			n.Type = AST_TOKEN_TYPE_MATH_OPERATION;
			result.push_back(n);
			break;
		default:
			token.append(1, ch);
			break;
		}
	}

	if (token.size() != 0)
	{
		n.Data.Data = token;
		n.Data.Line = lineNumber;
		n.Type = AST_TOKEN_TYPE_UNKNOWN;
		result.push_back(n);
	}

	return result;

#undef ADD_TOKEN_AND_CLEAR
}

bool ASTBase::IsValidParamModifier(const std::string& modifier) const
{
	return modifier == "in" || modifier == "inout" || modifier == "out";
}

void ASTBase::SetPrintHandler(IASTPrintHandler* handler)
{
	m_Print = handler;
}

bool ASTBase::ParseResourcesBlock(ASTParsedTokens& tokens)
{
	return false;
}

bool ASTBase::ParseStructDefinition(ASTParsedTokens& tokens)
{
	return false;
}

bool ASTBase::ParseFunctionDefinition(ASTParsedTokens& tokens)
{
	return false;
}

bool ASTBase::IsFunctionDeclaration(ASTParsedTokens tokens)
{
	// Assume we're at a general identifier
	if (!tokens.Advance())
	{
		return false;
	}

	if (!tokens.Advance())
	{
		return false;
	}

	if (tokens.Current().Type == AST_TOKEN_TYPE_LEFT_PARENTHESIS)
	{
		// Probably is
		return true;
	}

	return false;
}

bool ASTBase::IsStructDefined(const std::string& name)
{
	for (size_t i = 0; i < m_Structs.size(); i++)
	{
		if (name == m_Structs[i])
		{
			return true;
		}
	}

	return false;
}
