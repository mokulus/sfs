#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "str_array.h"

str_array read_stdin_lines() {
	str_array la;
	str_array_init(&la);
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		if (nread != 0) {
			if (line[nread-1] == '\n')
				line[nread-1] = '\0';
			str_array_add(&la, line);
		} else {
			free(line);
		}
		line = NULL;
		len = 0;
	}
	free(line);
	return la;
}

void str_tolower(char *str) {
	for (; *str; ++str) {
		*str = (char)tolower(*str);
	}
}

str_array tokenize(const char *str, const char *delim) {
	str_array la;
	str_array_init(&la);
	char *dstr = strdup(str);
	if (!dstr)
		goto fail;
	char *s = dstr;
	char *tok = NULL;
	while ((tok = strtok(s, delim)) != NULL) {
		char *tokstr = strdup(tok);
		if (!tokstr)
			goto fail;
		str_tolower(tokstr);
		str_array_add(&la, tokstr);
		s = NULL;
	}
fail:
	free(dstr);
	return la;
}

int fuzzy_match(const char *str, str_array *tokens) {
	if (tokens->length == 0) { // match all when empty
		return 1;
	}
	int ret = 1;
	char *dstr = strdup(str);
	if (!dstr)
		goto fail;
	str_tolower(dstr);
	for (size_t i = 0; i < tokens->length; ++i) {
		if (!strstr(dstr, tokens->lines[i])) {
			ret = 0;
			break;
		}
	}
fail:
	free(dstr);
	return ret;
}

void update_matches(const char *input, str_array *current_matches) {
	str_array new_matches;
	str_array_init(&new_matches);
	str_array tokens = tokenize(input, " ");
	for (size_t i = 0; i < current_matches->length; ++i) {
		if (fuzzy_match(current_matches->lines[i], &tokens)) {
			str_array_add(&new_matches, current_matches->lines[i]);
		}
	}
	free(current_matches->lines);
	str_array_free(&tokens);
	*current_matches = new_matches;
}

void print_matches(const char *input, str_array *current_matches, size_t choice, size_t view_offset, const char *prompt, size_t max_lines, size_t max_cols) {
	move(0, 0);
	printw("%s%s\n", prompt, input);
	size_t i = view_offset;
	size_t counter = 0;
	for (; i < current_matches->length && counter < max_lines; ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n", max_cols, current_matches->lines[i]);
		if (i == choice)
			attroff(A_REVERSE);
	}
	move(0, (int)(strlen(input) + strlen(prompt)));
}

void update_choice(ssize_t diff, size_t *choice, size_t *view_offset, const str_array *current_matches, size_t max_lines) {
	if (current_matches->length == 0)
		return;
	if (diff < 0) {
		diff += (ssize_t)current_matches->length;
	}
	*choice = (*choice + (size_t)diff) % current_matches->length;
	size_t last_view_offset = current_matches->length - max_lines;
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
			fprintf(stderr, "usage: %s [-1] [-p prompt]\n", argv[0]);
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

	str_array input_lines = read_stdin_lines();
	str_array current_matches;
	str_array_init(&current_matches);
	for (size_t i = 0; i < input_lines.length; ++i) {
		str_array_add(&current_matches, input_lines.lines[i]);
	}
	char *output = NULL;

	size_t MAX_LINES = (size_t)LINES - 2;
	size_t MAX_COLS = (size_t)COLS - 1;
	char input[1024] = {0};
	int input_len = 0;
	int c;
	size_t choice = 0;
	size_t view_offset = 0;
	print_matches(input, &current_matches, choice, view_offset, prompt, MAX_LINES, MAX_COLS);
	while ((c = getch()) != EOF) {
		int should_break = 0;
		ssize_t choice_diff = 0;
		switch (c) {
		case KEY_BACKSPACE:
		case 0x7F:
		case '\b':
			input[--input_len] = '\0';
			if (input_len < 0)
				input_len = 0;
			free(current_matches.lines);
			str_array_init(&current_matches);
			for (size_t i = 0; i < input_lines.length; ++i) {
				str_array_add(&current_matches, input_lines.lines[i]);
			}
			update_matches(input, &current_matches);
			clear();
			break;
		case '\n':
			if (current_matches.length != 0) {
				output = strdup(current_matches.lines[choice]);
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
				input[input_len++] = (char)tolower(c);
				update_matches(input, &current_matches);
				clear();
			} else {
				clear();
				printw("unknown %d %#x\n", c, c);
				getch();
			}
		}
		if (should_break)
			break;
		update_choice(choice_diff, &choice, &view_offset, &current_matches, MAX_LINES);
		print_matches(input, &current_matches, choice, view_offset, prompt, MAX_LINES, MAX_COLS);
		if (select_only_match && current_matches.length == 1) {
			output = strdup(current_matches.lines[0]);
			break;
		}
	}
	free(current_matches.lines);
	str_array_free(&input_lines);
	endwin();
	delscreen(screen);
	free(prompt);
	fclose(tty_in);
	fclose(tty_out);
	if (output) {
		printf("%s\n", output);
		free(output);
		return 0;
	} else {
		return 1;
	}
}
