#include "matcher_display.hpp"

MatcherDisplay::MatcherDisplay(const std::vector<std::string> &lines,
			       std::string prompt)
    : prompt(prompt), lines(lines), tty(fopen("/dev/tty", "r+")),
      screen(newterm(NULL, tty, tty)) {
	set_term(screen);
	noecho();
	cbreak();
	scrollok(stdscr, TRUE);
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);
	set_escdelay(0);
}

MatcherDisplay::~MatcherDisplay() {
	endwin();
	delscreen(screen);
	fclose(tty);
}

size_t MatcherDisplay::get_max_lines() const {
	return static_cast<size_t>(LINES - 2);
}

size_t MatcherDisplay::get_max_columns() const {
	return static_cast<size_t>(COLS - 1);
}

void MatcherDisplay::print(const Matcher &matcher) const {
	const auto &pattern = matcher.get_pattern();
	const auto &matches = matcher.get_matches();
	move(0, 0);
	printw("%s%s\n", prompt.c_str(), pattern.c_str());
	for (size_t i = view_offset, counter = 0;
	     i < matches.size() && counter < get_max_lines();
	     ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n", get_max_columns(), lines[matches[i]].c_str());
		if (i == choice)
			attroff(A_REVERSE);
	}
	move(0, (int)(prompt.length() + pattern.length()));
}

std::optional<std::string>
MatcherDisplay::get_choice(const Matcher &matcher) const {
	const auto &matches = matcher.get_matches();
	if (matches.empty()) {
		return std::nullopt;
	} else {
		return lines[matches[choice]];
	}
}

void MatcherDisplay::update(ssize_t diff, const Matcher &matcher) {
	const auto &matches = matcher.get_matches();
	const auto matches_count = matches.size();
	if (matches_count == 0)
		return;
	if (diff < 0) {
		diff += (ssize_t)matches_count;
	}
	choice = (choice + (size_t)diff) % matches_count;
	size_t last_view_offset = matches_count - get_max_lines();
	size_t last_rel_index = get_max_lines() - 1;
	if (choice < view_offset) {
		view_offset = choice;
	}
	if (choice > view_offset + last_rel_index) {
		view_offset = choice - last_rel_index;
	}
	if (view_offset >= last_view_offset) {
		view_offset = last_view_offset;
	}
}
