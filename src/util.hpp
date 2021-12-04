#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <iostream>
#include <string>

constexpr unsigned char ctrl(unsigned char c) {
	return c & 0x1f;
}

std::string str_tolower(std::string s);
std::vector<std::string> read_lines(std::istream &in);
std::vector<std::string> tokenize(const std::string &str,
				  const std::string &delim);

#endif
