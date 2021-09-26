#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <algorithm>
#include <iostream>

std::string str_tolower(std::string s);
std::vector<std::string> read_lines(std::istream &in);
std::vector<std::string> tokenize(const std::string &str, const std::string &delim);

#endif
