
#include "Utils.h"
#include <iostream>
#include <string>

void Utils::convetToWChar(wchar_t* buffer, const char* text)
{
	size_t size = strlen(text) + 1;

	size_t outSize;
	mbstowcs_s(&outSize, buffer, size, text, size - 1);
};

std::string Utils::convertToString(wchar_t* text)
{
	std::wstring ws(text);
	std::string result(ws.begin(), ws.end());
	return result;
}

std::string Utils::narrow_string(std::wstring const& s, char default_char)
{
	if (s.empty())
	{
		return std::string();
	}

	std::locale LOCALE_RUS{ "Russian_Russia.1251" };

	std::ctype<wchar_t> const& facet = std::use_facet<std::ctype<wchar_t>>(LOCALE_RUS);

	wchar_t const* first = s.c_str();
	wchar_t const* last = first + s.size();

	std::vector<char> result(s.size());

	facet.narrow(first, last, default_char, &result[0]);

	return std::string(result.begin(), result.end());
}

std::string Utils::stringToJson(std::string&& result, std::string&& error)
{
	std::ostringstream s_ansewer;

	s_ansewer << '{' << ' ';
	s_ansewer << '"' << "result" << '"' << ':';
	s_ansewer << '"' << result << '"' << ',' << ' ';
	s_ansewer << '"' << "error" << '"' << ':';
	s_ansewer << '"' << error << '"';
	s_ansewer << ' ' << '}';

	return s_ansewer.str();
}
