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

std::vector<std::string> read_stdin_lines() {
	std::vector<std::string> lines;
	for (std::string line; std::getline(std::cin, line);) {
		lines.push_back(line);
	}
	return lines;
}

// https://en.cppreference.com/w/cpp/string/byte/tolower
std::string str_tolower(std::string s) {
    std::transform(s.begin(),
			s.end(),
			s.begin(),
			[](unsigned char c){ return std::tolower(c); });
	return s;
}

std::vector<std::string> tokenize(std::string str, std::string delim) {
	str = str_tolower(str);
	delim = str_tolower(delim);
	const std::regex delim_regex(delim);
	const auto begin = std::sregex_token_iterator(str.begin(), str.end(), delim_regex, -1);
	const auto end = std::sregex_token_iterator();
	return std::vector<std::string>(begin, end);
}

bool fuzzy_match(const std::string &str, const std::vector<std::string> &tokens) {
	bool has_all_matches = true;
	for (const auto& token : tokens) {
		if (str.find(token) == std::string::npos) {
			has_all_matches = false;
			break;
		}
	}
	return has_all_matches;
}

void update_matches(const std::string &input,
		std::vector<std::size_t> &current_matches,
		const std::vector<std::string> &all_matches) {
	std::vector<std::size_t> new_matches;
	const auto tokens = tokenize(input, " ");
	for (auto match_id : current_matches) {
		if (fuzzy_match(all_matches[match_id], tokens)) {
			new_matches.push_back(match_id);
		}
	}
	std::swap(new_matches, current_matches);
}

void print_matches(const std::string &input,
		const std::vector<std::size_t> &current_matches,
		const std::vector<std::string> &all_matches,
		size_t choice,
		size_t view_offset,
		const std::string &prompt,
		size_t max_lines,
		size_t max_cols) {
	move(0, 0);
	printw("%s%s\n", prompt.c_str(), input.c_str());
	size_t i = view_offset;
	size_t counter = 0;
	for (; i < current_matches.size() && counter < max_lines; ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n", max_cols, all_matches[current_matches[i]].c_str());
		if (i == choice)
			attroff(A_REVERSE);
	}
	move(0, (int)(input.length() + prompt.length()));
}

void update_choice(ssize_t diff,
		size_t *choice,
		size_t *view_offset,
		const std::vector<std::size_t> &current_matches,
		size_t max_lines) {
	if (current_matches.empty())
		return;
	if (diff < 0) {
		diff += (ssize_t)current_matches.size();
	}
	*choice = (*choice + (size_t)diff) % current_matches.size();
	size_t last_view_offset = current_matches.size() - max_lines;
	size_t last_rel_index = max_lines - 1;
	if (*choice < *view_offset) {
		*view_offset = *choice;
	}
	if (*choice > *view_offset + last_rel_index) {
		*view_offset = *choice - last_rel_index;
	}
	if (*view_offset >= last_view_offset) {
		*view_offset = last_view_offset;
	}
}

int main(int argc, char *argv[]) {
	char *prompt = strdup("");
	unsigned select_only_match = 0;
	int opt;
	while ((opt = getopt(argc, argv, "1p:")) != -1) {
		switch (opt) {
		case 'p':
			prompt = strdup(optarg);
			break;
		case '1':
			select_only_match = 1;
			break;
		default:
			std::cerr << fmt::format("usage: {} [-1] [-p prompt]\n", argv[0]);
			return 1;
		}
	}
	FILE *tty_in = fopen("/dev/tty", "r");
	FILE *tty_out = fopen("/dev/tty", "w");
	SCREEN *screen = newterm(NULL, tty_out, tty_in);
	set_term(screen);
	noecho();
	cbreak();
	scrollok(stdscr, TRUE);
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);
	set_escdelay(0);

	const auto input_lines = read_stdin_lines();
	const auto lower_input_lines = [&]() {
		std::vector<std::string> lower_input_lines;
		std::transform(input_lines.begin(),
				input_lines.end(),
				std::back_inserter(lower_input_lines),
				[](const auto &str) { return str_tolower(str); });
		return lower_input_lines;
	} ();
	std::vector<std::size_t> current_matches;
	for (std::size_t i = 0; i < input_lines.size(); ++i) {
		current_matches.push_back(i);
	}
	std::optional<std::string> output;
	size_t MAX_LINES = (size_t)LINES - 2;
	size_t MAX_COLS = (size_t)COLS - 1;
	std::string input;
	int c;
	size_t choice = 0;
	size_t view_offset = 0;
	print_matches(input, current_matches, input_lines, choice, view_offset, prompt, MAX_LINES, MAX_COLS);
	while ((c = getch()) != EOF) {
		int should_break = 0;
		ssize_t choice_diff = 0;
		switch (c) {
		case KEY_BACKSPACE:
		case 0x7F:
		case '\b':
			if (input.length() > 0)
				input.pop_back();
			current_matches.clear();
			for (std::size_t i = 0; i < input_lines.size(); ++i) {
				current_matches.push_back(i);
			}
			update_matches(input, current_matches, lower_input_lines);
			clear();
			break;
		case '\n':
			if (not current_matches.empty()) {
				output = input_lines[current_matches[choice]];
				should_break = 1;
			}
			break;
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
			MAX_LINES = (size_t)LINES - 3;
			MAX_COLS = (size_t)COLS - 1;
			break;
		default:
			if (isprint(c)) {
				input.push_back(static_cast<char>(tolower(c)));
				update_matches(input, current_matches, lower_input_lines);
				clear();
			} else {
				clear();
				printw("unknown %d %#x\n", c, c);
				getch();
			}
		}
		if (should_break)
			break;
		update_choice(choice_diff, &choice, &view_offset, current_matches, MAX_LINES);
		print_matches(input, current_matches, input_lines, choice, view_offset, prompt, MAX_LINES, MAX_COLS);
		if (select_only_match && current_matches.size() == 1) {
			output = input_lines[current_matches[0]];
			break;
		}
	}
	endwin();
	delscreen(screen);
	free(prompt);
	fclose(tty_in);
	fclose(tty_out);
	if (output) {
		std::cout << fmt::format("{}\n", *output);
		return 0;
	} else {
		return 1;
	}
}
