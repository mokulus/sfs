#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <iostream>
#include <string>

std::string str_tolower(std::string s);
std::vector<std::string> read_lines(std::istream &in);
std::vector<std::string> tokenize(const std::string &str,
				  const std::string &delim);

#endif
