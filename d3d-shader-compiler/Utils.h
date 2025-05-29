#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>


inline std::vector<std::string> SplitString(const std::string& InStr, const char Delimiter)
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

inline void RemoveWhiteSpace(std::string& Str)
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

inline void RemoveTabs(std::string& Str)
{
	size_t Loc = 0;
	while ((Loc = Str.find('\t')) != std::string::npos)
	{
		Str.erase(Loc, 1);
	}
}

inline void RemoveAllNonSpaceWhiteSpace(std::string& Str)
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

inline bool IsWhiteSpace(const std::string& TestStr)
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


// Parameter is called count for readability
inline std::string CutStringRange(std::string& InStr, const size_t Start, const size_t Count)
{
	std::string Result = InStr.substr(Start, Count);
	InStr.erase((size_t)Start, (size_t)Count);
	return Result;
}

inline std::string CutStringLocations(std::string& InStr, const size_t Start, const size_t End)
{
	if (!(End > Start))
	{
		return "";
	}
	std::string Result = InStr.substr(Start, End - Start);
	InStr.erase(InStr.begin() + Start, InStr.begin() + End);
	return Result;
}


inline std::string ReadEntireFile(const std::string& FileName)
{
	std::ifstream InStream(FileName);
	std::string Result;
	Result.assign(std::istreambuf_iterator<char>(InStream),
		std::istreambuf_iterator<char>());

	return Result;
}

inline std::vector<std::string> GetAllFilesInFolder(const std::string& FolderName) {
	std::vector<std::string> Result;

	for (auto& File : std::filesystem::directory_iterator(FolderName))
	{
		Result.push_back(File.path().filename().string());
	}

	return Result;
}