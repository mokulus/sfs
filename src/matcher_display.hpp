#ifndef MATCHER_DISPLAY_HPP
#define MATCHER_DISPLAY_HPP

#include "matcher.hpp"
#include <ncurses.h>
#include <optional>
#include <string>
#include <vector>

class MatcherDisplay {
      public:
	MatcherDisplay(const Matcher &matcher, std::string prompt = "");
	~MatcherDisplay();
	std::size_t get_max_lines() const;
	std::size_t get_max_columns() const;
	void print();
	std::optional<std::string> get_choice() const;
	void move_choice(long diff);

	std::string prompt;

      private:
	void update_view(std::size_t matches_count);

	const Matcher &matcher;
	FILE *tty;
	SCREEN *screen;
	std::size_t choice = 0;
	std::size_t view_offset = 0;
};

#endif
