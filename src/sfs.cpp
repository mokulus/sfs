#include <cctype>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <regex>
#include <optional>

#include <unistd.h>
#include <ncurses.h>

#include <fmt/core.h>

#include "matcher.hpp"
#include "matcher_display.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
	const auto input_lines = read_lines(std::cin);
	Matcher matcher(input_lines);
	MatcherDisplay display(input_lines);
	std::optional<std::string> output;

	unsigned select_only_match = 0;
	int opt;
	while ((opt = getopt(argc, argv, "1p:")) != -1) {
		switch (opt) {
		case 'p':
			display.prompt = optarg;
			break;
		case '1':
			select_only_match = 1;
			break;
		default:
			std::cerr << fmt::format("usage: {} [-1] [-p prompt]\n", argv[0]);
			return 1;
		}
	}

	int c;
	display.print(matcher);
	while ((c = getch()) != EOF) {
		int should_break = 0;
		ssize_t choice_diff = 0;
		switch (c) {
		case KEY_BACKSPACE:
		case 0x7F:
		case '\b':
			matcher.pop();
			clear();
			break;
		case '\n': {
			const auto& matches = matcher.get_matches();
			if (not matches.empty()) {
				output = display.get_choice(matcher);
				should_break = 1;
			}
			break;
		}
		case 0x1B: // escape
			should_break = 1;
			break;
		case KEY_UP:
			choice_diff = -1;
			break;
		case KEY_DOWN:
			choice_diff = +1;
			break;
		case KEY_PPAGE:
			choice_diff = -10;
			break;
		case KEY_NPAGE:
			choice_diff = +10;
			break;
		case KEY_RESIZE:
			endwin();
			refresh();
			clear();
			break;
		default:
			if (isprint(c)) {
				matcher.push(static_cast<char>(c));
				clear();
			} else {
				clear();
				printw("unknown %d %#x\n", c, c);
				getch();
			}
		}
		if (should_break)
			break;
		display.update(choice_diff, matcher);
		display.print(matcher);
		if (select_only_match && matcher.get_matches().size() == 1) {
			output = display.get_choice(matcher);
			break;
		}
	}
	if (output) {
		std::cout << fmt::format("{}\n", *output);
		return 0;
	} else {
		return 1;
	}
}
