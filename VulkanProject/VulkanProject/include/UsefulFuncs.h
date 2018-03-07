#pragma once

#include <string>

inline bool ends_with(std::string const & value, std::string const & ending)
{
	if (ending.size() > value.size()) return false;
		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/* Primarily a separation function...
*/
inline std::string findStringBegin(const std::string &str, const std::string delim)
{
	return str.substr(0U, str.find(delim, 0U) - 0U);
}
/* Primarily a separation function...
*/
inline std::string findStringEnd(const std::string &str, const std::string delim)
{
	std::string retStr;
	size_t end, start = 0U;
	do {
		end = str.find(delim, start);
		retStr = str.substr(start, end - start);
		//std::cout << retStr << std::endl;
		start = end + 1;
	} while (end != std::string::npos);
	return retStr;
}