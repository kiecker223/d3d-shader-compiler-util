#include "Utils.h"
#include <string.h>
#include <cstdarg>




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

std::string GetFileSuffix(const std::string& filename) {
	size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == filename.length() - 1) {
		return ""; // No extension or nothing after the dot
	}
	return filename.substr(dotPos + 1);
}

std::string ReadEntireFile(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "fkfkfkf" << std::endl;
	}
	std::ostringstream ss;
	ss << file.rdbuf();  // Read the whole file into the stream buffer
	return ss.str();     // Convert to std::string
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

void IPrintHandler::Error(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->ErrorImpl(buffer);
}

void IPrintHandler::Warn(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->WarnImpl(buffer);
}

void IPrintHandler::Message(const char* fmt, ...)
{
	char buffer[4096] = { };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 4096, fmt, args);
	va_end(args);

	this->MessageImpl(buffer);
}