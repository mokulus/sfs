#include <cctype>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include <ncurses.h>
#include <unistd.h>

#include <fmt/core.h>

#include "matcher.hpp"
#include "matcher_display.hpp"
#include "util.hpp"

struct Config {
	bool select_only_match = false;
	std::string prompt = "";
};

std::optional<std::string> match(const Config &config);
std::optional<Config> get_opts(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	auto config = get_opts(argc, argv);
	if (!config) {
		std::cerr << fmt::format("usage: {} [-1] [-p prompt]\n",
					 argv[0]);
		return 1;
	}
	auto output = match(*config);
	if (output) {
		std::cout << fmt::format("{}\n", *output);
		return 0;
	} else {
		return 1;
	}
}

std::optional<std::string> match(const Config &config) {
	const auto input_lines = read_lines(std::cin);
	Matcher matcher(input_lines);
	MatcherDisplay display(input_lines);
	display.print(matcher);
	int c;
	while ((c = getch()) != EOF) {
		ssize_t choice_diff = 0;
		switch (c) {
		case KEY_BACKSPACE:
		case 0x7F:
		case '\b':
			matcher.pop();
			break;
		case '\n':
			if (not matcher.get_matches().empty()) {
				return display.get_choice(matcher);
			}
			break;
		case 0x1B: // escape
			return std::nullopt;
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
			break;
		default:
			if (isprint(c)) {
				matcher.push(static_cast<char>(c));
			}
		}
		display.update(choice_diff, matcher);
		display.print(matcher);
		if (config.select_only_match && matcher.get_matches().size() == 1) {
			return display.get_choice(matcher);
		}
	}
	return std::nullopt;
}

std::optional<Config> get_opts(int argc, char *argv[]) {
	Config config;
	int opt;
	while ((opt = getopt(argc, argv, "1p:")) != -1) {
		switch (opt) {
		case 'p':
			config.prompt = optarg;
			break;
		case '1':
			config.select_only_match = true;
			break;
		default:
			return std::nullopt;
		}
	}
	return config;
}
