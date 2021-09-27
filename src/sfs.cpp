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
	MatcherDisplay display(matcher);
	display.print();
	int c;
	while ((c = getch()) != EOF) {
		switch (c) {
		case KEY_BACKSPACE:
		case 0x7F:
		case '\b':
			matcher.pop();
			break;
		case '\n':
			return display.get_choice();
		case 0x1B: // escape
			return std::nullopt;
		case KEY_UP:
			display.move_choice(-1);
			break;
		case KEY_DOWN:
			display.move_choice(1);
			break;
		case KEY_PPAGE:
			display.move_choice(-10);
			break;
		case KEY_NPAGE:
			display.move_choice(10);
			break;
		case KEY_RESIZE:
			break;
		default:
			if (std::isprint(c)) {
				matcher.push(static_cast<char>(c));
			}
		}
		display.print();
		if (config.select_only_match &&
		    matcher.get_matches().size() == 1) {
			return display.get_choice();
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
