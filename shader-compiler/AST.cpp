#include "AST.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>




static DefaultPrintHandler g_DefaultPrint;

std::vector<TOKEN> ExtractTokens(const std::string& fileData)
{
	std::vector<TOKEN> Result;
	std::istringstream Str(fileData);

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

	std::string fileData = ReadEntireFile(path.string());
	if (fileData.size() == 0)
	{
		return false;
	}

	std::string hlslNoComments = RemoveHLSLComments(fileData);

	std::vector<TOKEN> lines = ExtractTokens(hlslNoComments);
	ASTUnparsedTokens toks(lines);
	Parse(toks);
	return true;
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
	uint32_t scopeCt = 0;
	do
	{
		const ASTToken& t = tokens.Current();

		if (scopeCt > SCOPE_UNDERFLOW_CHECK)
		{
			m_Print->Error("Invalid syntax detected on line %d", tokens.Last().GetLine());
			return false;
		}

		switch (t.Type)
		{
		case AST_TOKEN_TYPE_GENERAL_IDENTIFIER:
		{
			if (scopeCt != 0)
			{
				break;
			}
			if (IsPipelineStatement(tokens))
			{
				if (!ParsePipelineStatement(tokens))
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
			if (scopeCt != 0)
			{
				break;
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
			else 
			{
				AdvanceToEndOfStatement(tokens);
			}
			// Likely a variable declaration
		} break;
		case AST_TOKEN_TYPE_STRUCT_KEYWORD:
		{
			if (scopeCt != 0)
			{
				break;
			}
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
			if (scopeCt != 0)
			{
				break;
			}
			std::string keyword = t.GetData();
			if (keyword == "groupshared" ||
				keyword == "uniform" ||
				keyword == "const")
			{
				// This is likely a variable declaration, advance
				// until the first semicolon
				AdvanceToEndOfStatement(tokens);
			}
			else if (keyword == "register")
			{
				if (!ParseRegisterStatement(tokens))
				{
					if (m_UnrecoverableError)
					{
						return false;
					}
				}
				AdvanceToEndOfStatement(tokens);
			}
		} break;
		case AST_TOKEN_TYPE_LEFT_CURLY:
		{
			scopeCt++;
		} break;
		case AST_TOKEN_TYPE_RIGHT_CURLY:
		{
			scopeCt--;
		} break;
		}


	} while (tokens.Advance());

	return true;
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

bool ASTBase::HasParsedFunction(const std::string& name) const
{
	for (const std::string& n : m_Funcs)
	{
		if (name == n)
		{
			return true;
		}
	}

	return false;
}

bool ASTBase::IsSystemType(const std::string& type) const
{
	if (type == "void" || type == "bool" || type == "matrix")
	{
		return true;
	}

	static std::regex r("^(int|uint|dword|half|double|float)([1-4](x[1-4])?)?$", std::regex_constants::ECMAScript);
	if (std::regex_search(type, r))
	{
		return true;
	}

	return false;
}

bool ASTBase::IsHLSLReservedWord(const std::string& word) const
{
	static std::string keywords[] = { 
		"AppendStructuredBuffer", "asm", "asm_fragmentBlendState", "bool", "break", 
		"Buffer", "ByteAddressBuffercase", "cbuffer", "centroid", "class", "column_major", 
		"compile", "compile_fragment", "CompileShader", "const", "continue", "ComputeShader", 
		"ConsumeStructuredBufferdefault", "DepthStencilState", "DepthStencilView", 
		"discard", "do", "double", "DomainShader", "dwordelse", "export", "externfalse", 
		"float", "for", "fxgroupGeometryShader", "groupsharedhalf", "Hullshaderif", 
		"in", "inline", "inout", "InputPatch", "int", "interfaceline", "lineadj", 
		"linear", "LineStreammatrix", "min16float", "min10float", "min16int", "min12int", 
		"min16uintnamespace", "nointerpolation", "noperspective", "NULLout", 
		"OutputPatchpackoffset", "pass", "pixelfragment", "PixelShader", "point", 
		"PointStream", "preciseRasterizerState", "RenderTargetView", "return", 
		"register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", 
		"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", 
		"RWTexture3Dsample", "sampler", "SamplerState", "SamplerComparisonState", 
		"shared", "snorm", "stateblock", "stateblock_state", "static", "string", "struct", 
		"switch", "StructuredBuffertbuffer", "technique", "technique10", "technique11", 
		"texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", 
		"Texture2DMS", "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", 
		"true", "typedef", "triangle", "triangleadj", "TriangleStreamuint", "uniform", 
		"unorm", "unsignedvector", "vertexfragment", "VertexShader", "void", "volatilewhile" 
	};

	for (uint32_t i = 0; i < (sizeof(keywords) / sizeof(std::string)); i++)
	{
		if (word == keywords[i])
		{
			return true;
		}
	}

	return false;
}

bool ASTBase::AdvanceToEndOfScope(ASTParsedTokens& tokens, uint32_t scopeCt)
{
	// lets make sure we are able to get some kind of
	// error messaging here
	const ASTToken& current = tokens.Current();

	bool result = false;

	while (tokens.Advance())
	{
		if (tokens.Current().Type == AST_TOKEN_TYPE_LEFT_CURLY)
		{
			scopeCt++;
		}
		else if (tokens.Current().Type == AST_TOKEN_TYPE_RIGHT_CURLY)
		{
			scopeCt--;
		}

		if (scopeCt == 0)
		{
			result = true;
			break;
		}

		// Check integer underflow case
		// Can this realistically happen?
		if (scopeCt > SCOPE_UNDERFLOW_CHECK)
		{
			m_Print->Error("Error parsing scope starting at token \"%s\" on line %d", current.GetData().c_str(), current.Data.Line);
			result = false;
			break;
		}
	}

	// So here's an interesting philisophical question....
	// Do I try to advance past the end of the scope?
	// After some consideration I'm going to elect "no"
	if (!result)
	{
		return false;
	}

	// Did we successfully get to the end of the scope
	if (scopeCt != 0)
	{
		m_Print->Error("Unmatched scope starting at token \"%s\" on line %d", current.GetDataPtr(), current.GetLine());
		return false;
	}

	return true;
}

void ASTBase::AdvanceToEndOfStatement(ASTParsedTokens& tokens)
{	
	while (tokens.Advance())
	{
		if (tokens.Current().Type == AST_TOKEN_TYPE_SEMICOLON)
		{
			tokens.Advance();
			break;
		}
	}
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
			n.Type = AST_TOKEN_TYPE_RIGHT_PARENTHESIS;
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
			// This is kinda funky code reuse
			if (i + 1 > statement.size() - 1)
			{
				n.Data.Data = "[";
				n.Data.Line = lineNumber;
				n.Type = AST_TOKEN_TYPE_LEFT_SQUARE;
				result.push_back(n);
			}
			else
			{
				if (statement[i + 1] != '[')
				{
					n.Data.Data = "[";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_LEFT_SQUARE;
					result.push_back(n);
				}
				else
				{
					n.Data.Data = "[[";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_DOUBLE_LEFT_SQUARE;
					result.push_back(n);
					i += 1;
				}
			}
			break;
		case ']':
			ADD_TOKEN_AND_CLEAR;
			// This is kinda funky code reuse
			if (i + 1 > statement.size() - 1)
			{
				n.Data.Data = "]";
				n.Data.Line = lineNumber;
				n.Type = AST_TOKEN_TYPE_RIGHT_SQUARE;
				result.push_back(n);
			}
			else
			{
				if (statement[i + 1] != ']')
				{
					n.Data.Data = "]";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_RIGHT_SQUARE;
					result.push_back(n);
				}
				else
				{
					n.Data.Data = "]]";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_DOUBLE_RIGHT_SQUARE;
					result.push_back(n);
					i += 1;
				}
			}
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
			// This is kinda funky code reuse
			if (i + 1 > statement.size() - 1)
			{
				n.Data.Data = ":";
				n.Data.Line = lineNumber;
				n.Type = AST_TOKEN_TYPE_COLON;
				result.push_back(n);
			}
			else
			{
				if (statement[i + 1] != '[')
				{
					n.Data.Data = ":";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_COLON;
					result.push_back(n);
				}
				else
				{
					n.Data.Data = "::";
					n.Data.Line = lineNumber;
					n.Type = AST_TOKEN_TYPE_DOUBLE_COLON;
					result.push_back(n);
					i += 1;
				}
			}
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
		};
	}

	if (token.size() != 0)
	{
		n.Data.Data = token;
		n.Data.Line = lineNumber;
		n.Type = AST_TOKEN_TYPE_GENERAL_IDENTIFIER;
		result.push_back(n);
	}

	return result;

#undef ADD_TOKEN_AND_CLEAR
}

bool ASTBase::IsValidParamModifier(const std::string& modifier) const
{
	return modifier == "in" || modifier == "inout" || modifier == "out";
}

void ASTBase::SetPrintHandler(IPrintHandler* handler)
{
	m_Print = handler;
}

bool ASTBase::ParseResourcesBlock(ASTParsedTokens& tokens)
{
	const ASTToken& current = tokens.Current();

	if (m_ResourcesBlockParsed)
	{
		// Do I return? Do I generate an error?
		// I can forsee a case where someone copy pasted
		// this twice
		return false;
	}

	if (current.GetData() != "Resources")
	{
		return false;
	}

	// Advance the token to see if we have an assignment operator
	if (!tokens.Advance())
	{
		m_Print->Error("Reached end of file when trying to parse resources block");
		m_UnrecoverableError = true;
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_EQUALS)
	{
		m_Print->Error("Expected \"=\" after \"Resources\" block declaration");
		AdvanceToEndOfScope(tokens);
		return false;
	}

	if (!tokens.Advance()) 
	{
		m_Print->Error("Unexpected end of resources block on line %d", current.GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_CURLY)
	{
		m_Print->Error("Resources block must be assigned like \"Resources = {\", got \"Resources = %s\" on line %d",
			tokens.Current().GetDataPtr(),
			tokens.Current().GetLine()
		);
		AdvanceToEndOfScope(tokens);
		return false;
	}

	const ASTToken& lCurly = tokens.Current();

	// Finally in our journey. We are in the actual resources block
	// We honestly don't really care about the different types
	// for the resources. Literally all we care about are the different
	// "register ( *[0-16] )" tokens
	// However we also need to reconstruct the resources block
	// for later compilation
	// Actual syntax validation will be done by the real compiler

	// 1 because we're going to be inside the resources scope
	uint32_t scopeCt = 1;
	while (tokens.Advance())
	{
		const ASTToken& cur = tokens.Current();

		switch (cur.Type)
		{
		case AST_TOKEN_TYPE_HLSL_KEYWORD:
		{
			if (cur.GetData() == "register")
			{
				if (!tokens.Advance())
				{
					m_Print->Error("Unexpected end of statement at \"%s\" on line %d", cur.GetDataPtr(), cur.GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				m_ResourcesBlockStr += cur.GetData();

				if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_PARENTHESIS)
				{
					m_Print->Error("register must be followed by \"(<register number>)\" on line %d", tokens.Current().GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				m_ResourcesBlockStr += "(";

				if (!tokens.Advance())
				{
					m_Print->Error("Unexpected end of statement at \"%s\" on line %d", 
						tokens.Current().GetDataPtr(), tokens.Current().GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
				{
					m_Print->Error("Unexpected syntax -> \"%s\" on line %d",
						tokens.Current().GetDataPtr(), tokens.Current().GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				const ASTToken& regSlotToken = tokens.Current();

				if (!tokens.Advance())
				{
					m_Print->Error("Unexpected end of statement at \"%s\" on line %d",
						tokens.Current().GetDataPtr(), tokens.Current().GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				if (tokens.Current().Type != AST_TOKEN_TYPE_RIGHT_PARENTHESIS)
				{
					m_Print->Error("Unexpected token \"%s\" at end of register statement on line %d",
						tokens.Current().GetDataPtr(), tokens.Current().GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				if (regSlotToken.GetData().size() != 2 || regSlotToken.GetData().size() != 3)
				{
					m_Print->Error("Invalid register slot type \"%s\" on line %d",
						regSlotToken.GetDataPtr(), regSlotToken.GetLine());
					AdvanceToEndOfScope(tokens, 1);
					return false;
				}

				std::string regSlot = regSlotToken.GetData();
				char regType = regSlot[0];
				std::string regNumStr = regSlot.substr(1);
				bool doContinue = false;

				for (char ch : regNumStr)
				{
					if (ch < '0' || ch > '9')
					{
						// From here we leave it up to the compiler to fail
						m_Print->Error("register slot must be one of {u, t, s, b} followed by a number. Got %s on line %d",
							regSlotToken.GetDataPtr(), regSlotToken.GetLine());
						AdvanceToEndOfStatement(tokens);
						doContinue = true;
						break;
					}
				}

				m_ResourcesBlockStr += regSlot;
				m_ResourcesBlockStr += ")";

				if (!doContinue)
				{
					return false;
				}

				uint32_t num = std::atol(regSlot.substr(1).c_str());

				switch (regType)
				{
				case 'u':
					if (num > m_Counts.NumUnorderedAccessViews)
					{
						m_Counts.NumUnorderedAccessViews = num;
					}
					break;
				case 's':
					if (num > m_Counts.NumSamplers)
					{
						m_Counts.NumSamplers = num;
					}
					break;
				case 't':
					if (num > m_Counts.NumShaderResourceViews)
					{
						m_Counts.NumShaderResourceViews = num;
					}
					break;
				case 'b':
					if (num > m_Counts.NumConstantBuffers)
					{
						m_Counts.NumConstantBuffers = num;
					}
					break;
				}
			}
		} break;
		case AST_TOKEN_TYPE_LEFT_CURLY:
		{
			m_ResourcesBlockStr += "{\n";
			scopeCt--;
		} break;
		case AST_TOKEN_TYPE_RIGHT_CURLY:
		{
			m_ResourcesBlockStr += "}\n";
			scopeCt--;
		} break;
		case AST_TOKEN_TYPE_LEFT_GATOR:
		{
			m_ResourcesBlockStr += "<";
			if (!tokens.Advance())
			{
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
			m_ResourcesBlockStr += tokens.Current().GetData();
		} break;
		case AST_TOKEN_TYPE_RIGHT_GATOR:
		{
			m_ResourcesBlockStr += "> ";
		} break;
		default:
			m_ResourcesBlockStr += " ";
			m_ResourcesBlockStr += tokens.Current().GetData();
			m_ResourcesBlockStr += " ";
			break;
		}

		if (scopeCt == 0)
		{
			break;
		}
		if (scopeCt > SCOPE_UNDERFLOW_CHECK)
		{
			m_Print->Error("Scope number integer underflow at token %s on line %d", cur.GetDataPtr(), cur.GetLine());
			m_UnrecoverableError = true;
			return false;
		}
	}

	if (scopeCt != 0 || scopeCt > SCOPE_UNDERFLOW_CHECK)
	{
		m_Print->Error("Unmatched \"{\" starting line %d", lCurly.GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	return true;
}

bool ASTBase::IsPipelineStatement(ASTParsedTokens& tokens)
{
	const ASTToken& token = tokens.Current();
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

bool ASTBase::ParsePipelineStatement(ASTParsedTokens& tokens)
{
	if (m_PipelineParsed)
	{
		return false;
	}

	m_PipelineBlockStart = tokens.Current().GetLine();

	// Because toks was able to get here, we can assume 
	// these will succeed
	tokens.Advance(); // tokens.Current() = '='
	tokens.Advance(); // tokens.Current() = '{'

	m_PipelineNode = std::make_shared<ASTInitializerList>();
	NameList emptyName;

	if (!ParseInitializerScope(tokens, emptyName, m_PipelineNode))
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

	m_PipelineBlockEnd = tokens.Current().GetLine();

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	m_PipelineParsed = true;
	return true;
}

bool ASTBase::ParseStructDefinition(ASTParsedTokens& tokens)
{
	if (tokens.Current().Type != AST_TOKEN_TYPE_STRUCT_KEYWORD)
	{
		return false;
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
	{
		m_Print->Error("Struct name is not a valid name %s on line %d",
			tokens.Current().GetDataPtr(),
			tokens.Current().GetLine());
		return false;
	}

	uint32_t scopeCt = 0;
	ASTStructDecl structDecl;

	structDecl.Name = tokens.Current().GetData();

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_CURLY)
	{
		m_Print->Error("Struct declaration must have a \"{\" after the struct name on line %d",
			tokens.Current().GetLine());
		return false;
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	for (;;)
	{
		ASTStructDecl::Member member;

		if (tokens.Current().Type == AST_TOKEN_TYPE_SEMICOLON)
		{
			break;
		}

		// I am going to assume best case scenario always here
		// I don't really like it but, if this is an HLSL keyword
		// and not a builtin datatype, I'm going to assume the best
		// and that its a modifier for this value
		// TODO: Check that this is a valid modifier
		if (tokens.Current().Type == AST_TOKEN_TYPE_HLSL_KEYWORD)
		{
			member.Modifier = tokens.Current().GetData();

			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}

		if (tokens.Current().Type != AST_TOKEN_TYPE_BUILTIN_DATATYPE)
		{
			if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
			{
				m_Print->Error("Invalid struct member type \"%s\" on line %d",
					tokens.Current().GetDataPtr(),
					tokens.Current().GetLine());
				m_UnrecoverableError = true;
				return false;
			}
		}

		if (!IsValidType(tokens.Current().GetData()))
		{
			m_Print->Error("Invalid data type on line %d. Got \"%s\"",
				tokens.Current().GetLine(),
				tokens.Current().GetDataPtr());
			m_UnrecoverableError = true;
			return false;
		}

		member.Type = tokens.Current().GetData();

		if (!tokens.Advance())
		{
			m_UnrecoverableError = true;
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			return false;
		}

		if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
		{
			m_Print->Error("Struct member \"%s\" is not a valid member name on line %d",
				tokens.Current().GetDataPtr(),
				tokens.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		std::string memberName = tokens.Current().GetData();

		if (structDecl.Members.find(memberName) != structDecl.Members.end())
		{
			m_Print->Error("Struct member redefinition \"%s\" on line %d",
				tokens.Current().GetDataPtr(),
				tokens.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		member.Name = memberName;

		if (!tokens.Advance())
		{
			m_UnrecoverableError = true;
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			return false;
		}
		
		if (tokens.Current().Type == AST_TOKEN_TYPE_COLON)
		{
			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}

			if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
			{
				m_Print->Error("Invalid semantic name \"%s\" on line %d",
					tokens.Current().GetDataPtr(),
					tokens.Current().GetLine());
				return false;
			}

			member.Semantic = tokens.Current().GetData();

			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}

			if (tokens.Current().Type != AST_TOKEN_TYPE_SEMICOLON)
			{
				m_Print->Error("Missing ';' on line %d", tokens.Current().GetLine());
				return false;
			}

			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}
		else if (tokens.Current().Type == AST_TOKEN_TYPE_SEMICOLON)
		{
			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}
		else
		{
			m_Print->Error("Struct member declaration must end with a ';' on line %d",
				tokens.Current().GetLine());
			return false;
		}

		structDecl.Members.insert(std::pair<std::string, ASTStructDecl::Member>(member.Name, member));
	}

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_SEMICOLON)
	{
		m_Print->Error("Missing ';' on line %d", tokens.Current().GetLine());
		return false;
	}

	m_Structs.push_back(structDecl.Name);
	m_StructsParsed.insert(std::pair<std::string, ASTStructDecl>(structDecl.Name, structDecl));

	return true;
}

bool ASTBase::ParseInitializerScope(ASTParsedTokens& tokens, const NameList& name, std::shared_ptr<ASTInitializerList> outList)
{
	if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_CURLY)
	{
		return false;
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	for (;;)
	{
		// Grab inital identifier
		const ASTToken& identifier = tokens.Current();
		if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
		{
			m_Print->Error("Syntax error: Expected identifier on line %d, got: \"%s\"",
				tokens.Current().GetLine(),
				tokens.Current().GetDataPtr());
			return false;
		}

		if (!tokens.Advance())
		{
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		NameList names;

		// If we have assignment
		if (tokens.Current().Type == AST_TOKEN_TYPE_EQUALS)
		{
			names.push_back(identifier.GetData());
		}
		// Build the "names" list. We support multiple
		// variables per assignment statement
		else if (tokens.Current().Type == AST_TOKEN_TYPE_COMMA)
		{
			names.push_back(identifier.GetData());

			for (;;)
			{
				if (!tokens.Advance())
				{
					m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
					m_UnrecoverableError = true;
					return false;
				}
				const ASTToken& id = tokens.Current();

				if (id.Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
				{
					m_Print->Error("Syntax error on line %d. Got '%s' expected 'identifier'",
						id.GetLine(),
						id.GetDataPtr());
					return false;
				}

				names.push_back(id.GetData());

				if (!tokens.Advance())
				{
					m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
					m_UnrecoverableError = true;
					return false;
				}

				if (tokens.Current().Type == AST_TOKEN_TYPE_COMMA)
				{
					continue;
				}
				else if (tokens.Current().Type == AST_TOKEN_TYPE_EQUALS)
				{
					break;
				}
				else
				{
					m_Print->Error("Syntax error. Expected '=' or ',' got '%s' on line %d",
						tokens.Current().GetDataPtr(),
						tokens.Current().GetLine());
					return false;
				}
			}
		}
		else
		{
			m_Print->Error("Syntax error, expected '=' or ',' got \"%s\" on line %d",
				tokens.Current().GetDataPtr(),
				tokens.Current().GetLine());
			return false;
		}

		// Advance past the "equals", should be checked by both cases when assigning the name
		if (!tokens.Advance())
		{
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			m_UnrecoverableError = true;
			return false;
		}

		// Parse the value
		if (tokens.Current().Type == AST_TOKEN_TYPE_LEFT_CURLY)
		{
			std::shared_ptr<ASTAssignment> assignment = std::make_shared<ASTAssignment>();
			std::shared_ptr<ASTInitializerList> list = std::make_shared<ASTInitializerList>();

			if (!ParseInitializerScope(tokens, names, list))
			{
				return false;
			}

			if (tokens.Current().Type != AST_TOKEN_TYPE_SEMICOLON)
			{
				m_Print->Error("Syntax error. Expected ';' got \"%s\" on line %d",
					tokens.Current().GetDataPtr(),
					tokens.Current().GetLine()
				);
				return false;
			}

			assignment->Names = names;
			assignment->Value = list;
			outList->Assignments.push_back(assignment);
		}
		else if (tokens.Current().Type == AST_TOKEN_TYPE_GENERAL_IDENTIFIER ||
			tokens.Current().Type == AST_TOKEN_TYPE_BUILTIN_DATATYPE) // Cover the "true", "false" case. If this is an error we can catch later
		{
			std::shared_ptr<ASTAssignment> assignment = std::make_shared<ASTAssignment>();
			assignment->Names = names;
			assignment->Value = std::make_shared<ASTAssignmentValue>(tokens.Current().GetData());
			outList->Assignments.push_back(assignment);

			if (!tokens.Advance())
			{
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				m_UnrecoverableError = true;
				return false;
			}
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

		// This is kinda hacky and I'll have to be careful on the initializer parsing
		// But I would assume if we have a right curly here we're at the end of the "Pipeline"
		// scope
		if (tokens.Current().Type == AST_TOKEN_TYPE_RIGHT_CURLY)
		{
			m_Print->Message("Finished parsing scope");
			if (!tokens.Advance()) // Advance to ';'
			{
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				m_UnrecoverableError = true;
				return false;
			}
			break;
		}
	}

	return true;
}

bool ASTBase::ParseRegisterStatement(ASTParsedTokens& tokens)
{
	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of statement at \"%s\" on line %d",
			tokens.Current().GetDataPtr(), tokens.Current().GetLine());
		AdvanceToEndOfStatement(tokens);
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_PARENTHESIS)
	{
		m_Print->Error("register must be followed by \"(<register number>)\" on line %d", tokens.Current().GetLine());
		AdvanceToEndOfStatement(tokens);
		return false;
	}

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of statement at \"%s\" on line %d",
			tokens.Current().GetDataPtr(), tokens.Current().GetLine());
		AdvanceToEndOfStatement(tokens);
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
	{
		m_Print->Error("Unexpected syntax -> \"%s\" on line %d",
			tokens.Current().GetDataPtr(), tokens.Current().GetLine());
		AdvanceToEndOfStatement(tokens);
		return false;
	}

	const ASTToken& regSlotToken = tokens.Current();

	if (!tokens.Advance())
	{
		m_Print->Error("Unexpected end of statement at \"%s\" on line %d",
			tokens.Current().GetDataPtr(), tokens.Current().GetLine());
		m_UnrecoverableError = true;
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_RIGHT_PARENTHESIS)
	{
		if (tokens.Current().Type == AST_TOKEN_TYPE_COMMA)
		{
			if (!tokens.Advance())
			{
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				m_UnrecoverableError = true;
				return false;
			}
			if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
			{
				m_Print->Error("Syntax error on line %d. Expected \"space<number>\" got %s",
					tokens.Current().GetLine(),
					tokens.Current().GetDataPtr());
				return false;
			}
			if (!tokens.Advance())
			{
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				m_UnrecoverableError = true;
				return false;
			}
			if (tokens.Current().Type != AST_TOKEN_TYPE_RIGHT_PARENTHESIS)
			{
				m_Print->Error("Unexpected syntax on line %d. Expected ')' got '%s'",
					tokens.Current().GetLine(),
					tokens.Current().GetDataPtr());
				return false;
			}
		}
		else
		{
			m_Print->Error("Unexpected token \"%s\" at end of register statement on line %d",
				tokens.Current().GetDataPtr(), tokens.Current().GetLine());
			AdvanceToEndOfStatement(tokens);
			return false;
		}
	}

	if (regSlotToken.GetData().size() != 2 && regSlotToken.GetData().size() != 3)
	{
		m_Print->Error("Invalid register slot type \"%s\" on line %d",
			regSlotToken.GetDataPtr(), regSlotToken.GetLine());
		AdvanceToEndOfStatement(tokens);
		return false;
	}

	std::string regSlot = regSlotToken.GetData();
	char regType = regSlot[0];
	std::string regNumStr = regSlot.substr(1);
	bool doContinue = true;

	for (char ch : regNumStr)
	{
		if (ch < '0' || ch > '9')
		{
			// From here we leave it up to the compiler to fail
			m_Print->Error("register slot must be one of {u, t, s, b} followed by a number. Got %s on line %d",
				regSlotToken.GetDataPtr(), regSlotToken.GetLine());
			AdvanceToEndOfStatement(tokens);
			doContinue = false;
			break;
		}
	}

	if (!doContinue)
	{
		return false;
	}

	uint32_t num = std::atol(regSlot.substr(1).c_str());
	num++;

	switch (regType)
	{
	case 'u':
		if (num > m_Counts.NumUnorderedAccessViews)
		{
			m_Counts.NumUnorderedAccessViews = num;
		}
		break;
	case 's':
		if (num > m_Counts.NumSamplers)
		{
			m_Counts.NumSamplers = num;
		}
		break;
	case 't':
		if (num > m_Counts.NumShaderResourceViews)
		{
			m_Counts.NumShaderResourceViews = num;
		}
		break;
	case 'b':
		if (num > m_Counts.NumConstantBuffers)
		{
			m_Counts.NumConstantBuffers = num;
		}
		break;
	}

	return true;
}


bool ASTBase::ParseFunctionDefinition(ASTParsedTokens& tokens)
{
	ASTFunctionDecl funcDecl;

	funcDecl.ReturnType = tokens.Current().GetData();

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
	{
		m_Print->Error("Invalid function name on line %d. Got \"%s\"",
			tokens.Current().GetLine(),
			tokens.Current().GetDataPtr());
		return false;
	}

	funcDecl.Name = tokens.Current().GetData();

	if (HasParsedFunction(funcDecl.Name))
	{
		m_Print->Error("Function redefinition \"%s\" on line %d",
			funcDecl.Name,
			tokens.Current().GetLine());
		return false;
	}

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	// This shouldn't be possible I think?
	if (tokens.Current().Type != AST_TOKEN_TYPE_LEFT_PARENTHESIS)
	{
		m_Print->Error("Invalid syntax for function declaration on line %d", tokens.Current().GetLine());
		return false;
	}

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	for (;;)
	{
		ASTFunctionDecl::Param param;

		if (tokens.Current().Type == AST_TOKEN_TYPE_RIGHT_PARENTHESIS)
		{
			break;
		}

		// Do a catch all to make sure that we can proceed
		if ((
			tokens.Current().Type != AST_TOKEN_TYPE_PARAM_MODIFIER &&
			tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER &&
			tokens.Current().Type != AST_TOKEN_TYPE_BUILTIN_DATATYPE
			) ||
			tokens.Current().Type == AST_TOKEN_TYPE_LEFT_PARENTHESIS)
		{
			m_Print->Warn("Function parsing breakout case");
			break;
		}

		if (tokens.Current().Type == AST_TOKEN_TYPE_PARAM_MODIFIER)
		{
			param.Modifier = tokens.Current().GetData();

			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}

		if (tokens.Current().Type == AST_TOKEN_TYPE_GENERAL_IDENTIFIER ||
			tokens.Current().Type == AST_TOKEN_TYPE_BUILTIN_DATATYPE)
		{
			if (!IsValidType(tokens.Current().GetData()))
			{
				m_Print->Error("Invalid data type in function parameter for function %s. Type: %s on line %d",
					funcDecl.Name.c_str(),
					tokens.Current().GetDataPtr(),
					tokens.Current().GetLine());
				return false;
			}

			param.Type = tokens.Current().GetData();
		}
		else
		{
			m_Print->Error("Invalid function parameter type %s for function %s on line %d",
				tokens.Current().GetDataPtr(),
				funcDecl.Name.c_str(),
				tokens.Current().GetLine());
			return false;
		}

		if (!tokens.Advance())
		{
			m_UnrecoverableError = true;
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			return false;
		}

		if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
		{
			m_Print->Error("Invalid parameter name %s for function %s on line %d",
				tokens.Current().GetDataPtr(),
				funcDecl.Name.c_str(),
				tokens.Current().GetLine());
			return false;
		}

		param.Name = tokens.Current().GetData();

		if (!tokens.Advance())
		{
			m_UnrecoverableError = true;
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			return false;
		}

		if (tokens.Current().Type == AST_TOKEN_TYPE_COLON)
		{
			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}

			if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
			{
				m_Print->Error("Invalid semantic name %s for function %s on line %d",
					tokens.Current().GetDataPtr(),
					funcDecl.Name.c_str(),
					tokens.Current().GetLine());
				return false;
			}

			param.Semantic = tokens.Current().GetData();

			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}
		else if (tokens.Current().Type == AST_TOKEN_TYPE_COMMA)
		{
			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}
		}
		// We're going to make an assumption that
		// if a variable has an associated semantic,
		// it probably won't have an initializer
		else if (tokens.Current().Type == AST_TOKEN_TYPE_EQUALS)
		{
			if (!tokens.Advance())
			{
				m_UnrecoverableError = true;
				m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
				return false;
			}

			// We don't care about initializers, what we're going to do
			// is just advance to either the first uncontained comma
			// or the left parenthesis

			// p for parenthesis
			int pScopeCt = 0;
			// c for curly braces
			int cScopeCt = 0;
			for (;;)
			{
				if (tokens.Current().Type == AST_TOKEN_TYPE_COMMA)
				{
					if (pScopeCt <= 0 && cScopeCt <= 0)
					{
						break;
					}
				}
				else if (tokens.Current().Type == AST_TOKEN_TYPE_LEFT_PARENTHESIS)
				{
					pScopeCt++;
				}
				else if (tokens.Current().Type == AST_TOKEN_TYPE_LEFT_CURLY)
				{
					cScopeCt++;
				}
				else if (tokens.Current().Type == AST_TOKEN_TYPE_RIGHT_PARENTHESIS)
				{
					pScopeCt--;
				}
				else if (tokens.Current().Type == AST_TOKEN_TYPE_RIGHT_CURLY)
				{
					cScopeCt--;
				}

				if (pScopeCt < 0 || cScopeCt < 0)
				{
					m_Print->Error("Invalid syntax in parameter initializer on line %d", tokens.Current().GetLine());
					return false;
				}

				if (!tokens.Advance())
				{
					m_UnrecoverableError = true;
					m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
					return false;
				}
			}
		}

		funcDecl.Params.push_back(param);
	}

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	if (tokens.Current().Type == AST_TOKEN_TYPE_COLON)
	{
		if (!tokens.Advance())
		{
			m_UnrecoverableError = true;
			m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
			return false;
		}

		if (tokens.Current().Type != AST_TOKEN_TYPE_GENERAL_IDENTIFIER)
		{
			m_Print->Error("Invalid return semantic for function %s. Got %s on line %d",
				funcDecl.Name.c_str(),
				tokens.Current().GetDataPtr(),
				tokens.Current().GetLine());
			return false;
		}

		funcDecl.ReturnSemantic = tokens.Current().GetData();
	}

	if (!tokens.Advance())
	{
		m_UnrecoverableError = true;
		m_Print->Error("Unexpected end of file on line %d", tokens.Current().GetLine());
		return false;
	}

	m_Funcs.push_back(funcDecl.Name);
	m_FuncsParsed.insert(std::pair<std::string, ASTFunctionDecl>(funcDecl.Name, funcDecl));

	return true;

Failure:

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
