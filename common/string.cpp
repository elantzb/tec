#include "string.hpp"

#include <algorithm>
#include <regex>

#include <locale>
#if !defined(__linux__)
#include <codecvt>
#else
#include <cstdlib>
#include <cstring>
#endif

namespace tec {

// Encoding conversion stuff thanks to : http://stackoverflow.com/a/18374698

// setup converter between UTF-8 <-> wstring encoding
#if !defined(__linux__)
using convert_utf8 = std::codecvt_utf8<wchar_t>;
#endif

std::string utf8_encode(const std::wstring& wstr) {
	if (wstr.empty()) {
		return std::string();
	}
#if !defined(__linux__)
	std::wstring_convert<convert_utf8, wchar_t> converter;

	// use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.to_bytes(wstr);
#else
	// Workaround for stdlibc++ not implementing yet codecvt lib
	std::string curLocale = std::setlocale(LC_ALL, "");
	const wchar_t* _Source = wstr.c_str();
	size_t _Dsize = std::wcstombs(NULL, _Source, 0) + 1;
	char* _Dest = new char[_Dsize];
	std::memset(_Dest, 0, _Dsize);
	std::wcstombs(_Dest, _Source, _Dsize);
	std::string result = _Dest;
	delete[] _Dest;
	std::setlocale(LC_ALL, curLocale.c_str());
	return result;
#endif
}

std::wstring utf8_decode(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}
#if !defined(__linux__)
	std::wstring_convert<convert_utf8, wchar_t> converter;

	return converter.from_bytes(str);
#else
	// Workaround for stdlibc++ not implementing yet codecvt lib
	// Take from http://stackoverflow.com/a/25142564/770931
	std::string curLocale = std::setlocale(LC_ALL, "");
	const char* _Source = str.c_str();
	size_t _Dsize = std::mbstowcs(NULL, _Source, 0) + 1;
	wchar_t* _Dest = new wchar_t[_Dsize];
	std::wmemset(_Dest, 0, _Dsize);
	std::mbstowcs(_Dest, _Source, _Dsize);
	std::wstring result = _Dest;
	delete[] _Dest;
	std::setlocale(LC_ALL, curLocale.c_str());
	return result;
#endif
}

std::vector<std::string> SplitString(std::string args, std::string deliminator) {
	auto regexz = std::regex(deliminator);
	return {std::sregex_token_iterator(args.begin(), args.end(), regexz, -1), std::sregex_token_iterator()};
}

size_t get_line_end(std::string _str, size_t _offset) {
	size_t ret = _str.find("\n", _offset);

	if (_str.c_str()[ret-1] == '\r')
		ret -= 1;

	return ret;
}

size_t get_line_start(std::string _str, size_t _offset) {
	size_t ret = _offset;
	
	while(_str.c_str()[ret] == '\r' || _str.c_str()[ret] == '\n')
		ret += 1;

	return ret;
}

} // namespace tec
