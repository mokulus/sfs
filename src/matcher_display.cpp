#include "matcher_display.hpp"

MatcherDisplay::MatcherDisplay(const Matcher &matcher, std::string prompt)
    : prompt(prompt), matcher(matcher), tty(fopen("/dev/tty", "r+")),
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

std::size_t MatcherDisplay::get_max_lines() const {
	return static_cast<std::size_t>(LINES - 2);
}

std::size_t MatcherDisplay::get_max_columns() const {
	return static_cast<std::size_t>(COLS - 1);
}

void MatcherDisplay::print() {
	const auto &pattern = matcher.get_pattern();
	const auto &matches = matcher.get_matches();
	update_view(matches.size());
	erase();
	move(0, 0);
	printw("%s%s\n", prompt.c_str(), pattern.c_str());
	for (std::size_t i = view_offset, counter = 0;
	     i < matches.size() && counter < get_max_lines();
	     ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n",
		       get_max_columns(),
		       matcher.lines[matches[i]].c_str());
		if (i == choice)
			attroff(A_REVERSE);
	}
	move(0, (int)(prompt.length() + pattern.length()));
	refresh();
}

std::optional<std::string> MatcherDisplay::get_choice() const {
	const auto &matches = matcher.get_matches();
	if (matches.empty()) {
		return std::nullopt;
	} else {
		return matcher.lines[matches[choice]];
	}
}

void MatcherDisplay::move_choice(long diff) {
	const auto match_count = matcher.get_matches().size();
	if (match_count == 0)
		return;
	while (diff < 0) {
		diff += match_count;
	}
	choice += static_cast<size_t>(diff);
	update_view(match_count);
}

void MatcherDisplay::update_view(std::size_t match_count) {
	if (match_count == 0)
		return;
	choice %= match_count;
	std::size_t last_view_offset = match_count - get_max_lines();
	std::size_t last_rel_index = get_max_lines() - 1;
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
