
#pragma once
#include <types.h>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <sstream>

class Utils
{
public:
	static void convetToWChar(wchar_t* buffer, const char* text);
	static std::string convertToString(wchar_t* text);
	static std::string narrow_string(std::wstring const& s, char default_char = '?');
	static std::string stringToJson(std::string&& result, std::string&& error = "None");
};