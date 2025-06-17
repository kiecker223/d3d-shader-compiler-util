#include "Utils.h"
#include <string.h>


std::vector<std::string> SplitString(const std::string& InStr, const char Delimiter)
{
	std::istringstream Str(InStr);

	std::vector<std::string> Result;

	std::string Buff;
	while (std::getline(Str, Buff, Delimiter))
	{
		Result.push_back(Buff);
	}
	return Result;
}

std::vector<std::string> SplitString(const std::string& InStr, const std::string& Delims)
{
	std::string currentTok;
	std::vector<std::string> result;

	for (char ch : InStr)
	{
		bool isDelim = false;

		for (uint32_t i = 0; i < Delims.size(); i++)
		{
			if (ch == Delims[i])
			{
				isDelim = true;
			}
		}

		if (isDelim)
		{
			if (currentTok != "")
			{
				result.push_back(currentTok);
			}

			currentTok = "";
		}
		else
		{
			currentTok.append(1, ch);
		}
	}

	if (currentTok != "")
	{
		result.push_back(currentTok);
	}

	return result;
}

std::vector<std::string> TokenizeLine(const std::string& line) {
	std::vector<std::string> tokens;
	std::string token;

	for (char ch : line) {
		if (std::isspace(static_cast<unsigned char>(ch))) {
			if (!token.empty()) {
				tokens.push_back(token);
				token.clear();
			}
		}
		else {
			token += ch;
		}
	}

	if (!token.empty()) {
		tokens.push_back(token);
	}

	return tokens;
}

std::vector<std::string> Tokenize(const std::string& fileData) {
	std::vector<std::string> allTokens;
	std::stringstream file(fileData);

	std::string line;
	while (std::getline(file, line)) {
		std::vector<std::string> lineTokens = TokenizeLine(line);
		allTokens.insert(allTokens.end(), lineTokens.begin(), lineTokens.end());
	}

	return allTokens;
}

void RemoveWhiteSpace(std::string& Str)
{
	size_t Loc = 0;
	while ((Loc = Str.find(' ')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
	while ((Loc = Str.find('\n')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
	while ((Loc = Str.find('\t')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
}

void RemoveTabs(std::string& Str)
{
	size_t Loc = 0;
	while ((Loc = Str.find('\t')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
}

void RemoveAllNonSpaceWhiteSpace(std::string& Str)
{
	size_t Loc = 0;
	while ((Loc = Str.find('\n')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
	while ((Loc = Str.find('\t')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
}

bool IsWhiteSpace(const std::string& TestStr)
{
	bool bWhiteSpace = false;
	char C;
	for (uint32_t i = 0; i < TestStr.length(); i++)
	{
		C = TestStr[i];
		if (C == ' ' || C == '\n' || C == '\t')
		{
			bWhiteSpace = true;
		}
		else
		{
			bWhiteSpace = false;
			break;
		}
	}
	return bWhiteSpace;
}

bool IsWhiteSpace(char ch)
{
	return ch == ' ' || ch == '\n' || ch == '\t';
}

// Parameter is called count for readability
std::string CutStringRange(std::string& InStr, const size_t Start, const size_t Count)
{
	std::string Result = InStr.substr(Start, Count);
	InStr.erase((size_t)Start, (size_t)Count);
	return Result;
}

std::string CutStringLocations(std::string& InStr, const size_t Start, const size_t End)
{
	if (!(End > Start))
	{
		return "";
	}
	std::string Result = InStr.substr(Start, End - Start);
	InStr.erase(InStr.begin() + Start, InStr.begin() + End);
	return Result;
}

std::string ReadEntireFile(const std::string& FileName)
{
	std::ifstream InStream(FileName);
	std::string Result;
	Result.assign(std::istreambuf_iterator<char>(InStream),
		std::istreambuf_iterator<char>());

	return Result;
}

std::vector<std::string> GetAllFilesInFolder(const std::string& FolderName) {
	std::vector<std::string> Result;

	for (auto& File : std::filesystem::directory_iterator(FolderName))
	{
		Result.push_back(File.path().filename().string());
	}

	return Result;
}

void RemoveComments(std::string& OutStr)
{
	size_t CurrentLoc = 0;
	for (;;)
	{
		size_t CommentLoc = OutStr.find("//", CurrentLoc);
		size_t EndLineLoc = OutStr.find("\n", CommentLoc);

		if (CommentLoc != std::string::npos)
		{
			CutStringLocations(OutStr, CommentLoc, EndLineLoc);
		}
		else
		{
			break;
		}
		CurrentLoc = CommentLoc;
	}

	CurrentLoc = 0;
	for (;;)
	{
		size_t CommentStartLoc = OutStr.find("/*", 0);
		size_t CommentEndLoc = OutStr.find("*/", 0);
		if (CommentStartLoc != std::string::npos)
		{
			CutStringLocations(OutStr, CommentStartLoc, CommentEndLoc + 2);
		}
		else
		{
			break;
		}
	}
}

std::string RemoveHLSLComments(const std::string& source)
{
	std::string result;
	bool inSingleLineComment = false;
	bool inMultiLineComment = false;

	for (size_t i = 0; i < source.length(); ++i) {
		if (!inSingleLineComment && !inMultiLineComment) {
			if (source[i] == '/' && i + 1 < source.length()) {
				if (source[i + 1] == '/') {
					inSingleLineComment = true;
					++i; // skip the next '/'
					continue;
				}
				else if (source[i + 1] == '*') {
					inMultiLineComment = true;
					++i; // skip the next '*'
					continue;
				}
			}
			result += source[i];
		}
		else if (inSingleLineComment) {
			if (source[i] == '\n') {
				inSingleLineComment = false;
				result += '\n'; // preserve newline
			}
		}
		else if (inMultiLineComment) {
			if (source[i] == '*' && i + 1 < source.length() && source[i + 1] == '/') {
				inMultiLineComment = false;
				++i; // skip the '/'
			}
			else if (source[i] == '\n') {
				result += '\n'; // preserve newline
			}
		}
	}

	return result;
}