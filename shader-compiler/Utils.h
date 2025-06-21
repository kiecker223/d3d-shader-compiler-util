#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

bool IsWhiteSpace(const std::string& TestStr);

bool IsWhiteSpace(char ch);

std::string GetFileSuffix(const std::string& filename);

std::string ReadEntireFile(const std::string& filename);

std::string RemoveHLSLComments(const std::string& source);

class IPrintHandler
{
public:

	virtual void ErrorImpl(const char* message) = 0;
	virtual void WarnImpl(const char* message) = 0;
	virtual void MessageImpl(const char* message) = 0;

	void Error(const char* fmt, ...);
	void Warn(const char* fmt, ...);
	void Message(const char* fmt, ...);
};

class DefaultPrintHandler : public IPrintHandler
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