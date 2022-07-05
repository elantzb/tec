#pragma once
/**
* String handling stuff
*/

#include <string>
#include <vector>

namespace tec {

/**
	* \brief Convert a wide Unicode string to an UTF8 string
	*
	* \param wstr wide string on unicode. On Windows -> UTF16. On Linux -> UTF32
	* \return std::string with UTF-8 encoding. Empty string if fails
	*/
std::string utf8_encode(const std::wstring& wstr);

/**
	* \brief Convert an UTF8 string to a wide Unicode String
	*
	* \param str UTF8 string
	* \return std::wstring on unicode system encoding. Empty string if fails
	*/
std::wstring utf8_decode(const std::string& str);

std::vector<std::string> SplitString(std::string args, std::string deliminator = " ");

/**
	* \brief Seek the index of the first line ending in the string
	*
	* \param _str the string in which to search
	* \param _offset index to start searching from in _str
	* \return size_t index of first line ending after _offset
	*/
size_t get_line_end(std::string _str, size_t _offset = 0U);

/**
	* \brief Seek the index of the first non-line-ending character in the string
	*
	* \param _str the string in which to search
	* \param _offset index to start searching from in _str
	* \return size_t index of first non-line-ending character after _offset
	*/
size_t get_line_start(std::string _str, size_t _offset = 0U);

} // namespace tec
