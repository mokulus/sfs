#ifndef MATCHER_DISPLAY_HPP
#define MATCHER_DISPLAY_HPP

#include "matcher.hpp"
#include <ncurses.h>
#include <optional>
#include <string>
#include <vector>

class MatcherDisplay {
      public:
	MatcherDisplay(const std::vector<std::string> &lines,
		       std::string prompt = "");
	~MatcherDisplay();
	size_t get_max_lines() const;
	size_t get_max_columns() const;
	void print(const Matcher &matcher) const;
	std::optional<std::string> get_choice(const Matcher &matcher) const;
	void update(ssize_t diff, const Matcher &matcher);

	std::string prompt;

      private:
	const std::vector<std::string> &lines;
	FILE *tty;
	SCREEN *screen;
	size_t choice = 0;
	size_t view_offset = 0;
};

#endif
