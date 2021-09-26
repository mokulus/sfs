#include "util.hpp"
#include <regex>

// https://en.cppreference.com/w/cpp/string/byte/tolower
std::string str_tolower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
	return s;
}

std::vector<std::string> read_lines(std::istream &in) {
	std::vector<std::string> lines;
	for (std::string line; std::getline(in, line);) {
		lines.push_back(line);
	}
	return lines;
}

std::vector<std::string> tokenize(const std::string &str,
				  const std::string &delim) {
	const std::regex delim_regex(delim);
	const auto begin =
	    std::sregex_token_iterator(str.begin(), str.end(), delim_regex, -1);
	const auto end = std::sregex_token_iterator();
	return std::vector<std::string>(begin, end);
}
