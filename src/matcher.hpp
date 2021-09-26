#ifndef MATCHER_HPP
#define MATCHER_HPP

#include <iostream>
#include <string>
#include <vector>

class Matcher {
      public:
	Matcher(const std::vector<std::string> &lines);
	void pop();
	void push(char c);
	const std::string &get_pattern() const;
	const std::vector<std::size_t> &get_matches() const;

      private:
	static std::vector<std::string>
	lines_to_lower(const std::vector<std::string> &lines);
	static bool fuzzy_match(const std::string &str,
				const std::vector<std::string> &tokens);
	void update_matches();
	void reset_matches();

	const std::vector<std::string> &lines;
	const std::vector<std::string> lower_lines;
	std::vector<std::size_t> matches;
	std::string pattern;
};

#endif
