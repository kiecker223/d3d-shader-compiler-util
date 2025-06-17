#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>


std::vector<std::string> SplitString(const std::string& InStr, const char Delimiter);

std::vector<std::string> SplitString(const std::string& InStr, const std::string& Delims);

std::vector<std::string> TokenizeLine(const std::string& line);

std::vector<std::string> Tokenize(const std::string& fileData);

void RemoveWhiteSpace(std::string& Str);

void RemoveTabs(std::string& Str);

void RemoveAllNonSpaceWhiteSpace(std::string& Str);

bool IsWhiteSpace(const std::string& TestStr);

bool IsWhiteSpace(char ch);


// Parameter is called count for readability
std::string CutStringRange(std::string& InStr, const size_t Start, const size_t Count);

std::string CutStringLocations(std::string& InStr, const size_t Start, const size_t End);


std::string ReadEntireFile(const std::string& FileName);

std::vector<std::string> GetAllFilesInFolder(const std::string& FolderName);

void RemoveComments(std::string& OutStr);

std::string RemoveHLSLComments(const std::string& source);