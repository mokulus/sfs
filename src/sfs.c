#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
	char *str;
	size_t length;
} lstr;

typedef struct {
	lstr *lines;
	size_t length;
	size_t capacity;
} lstr_array;

void lstr_array_init(lstr_array *array) {
	array->capacity = 4;
	array->length = 0;
	array->lines = calloc(array->capacity, sizeof(*array->lines));
}

unsigned lstr_array_add(lstr_array *array, lstr l) {
	array->length++;
	if (array->length > array->capacity) {
		size_t new_cap = array->capacity * 3 / 2;
		void *p = realloc(array->lines, sizeof(*array->lines) * new_cap);
		if (!p) {
			array->length--;
			return 0;
		}
		array->lines = p;
		array->capacity = new_cap;
	}
	array->lines[array->length-1] = l;
	return 1;
}

void lstr_array_free(lstr_array *array) {
	for (size_t i = 0; i < array->length; ++i) {
		free(array->lines[i].str);
	}
	free(array->lines);
}

lstr_array read_stdin_lines() {
	lstr_array la;
	lstr_array_init(&la);
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		if (nread != 0 && line[nread-1] == '\n') {
			line[nread-1] = '\0';
			nread--;
		}
		if (nread != 0) {
			lstr l = {line, (size_t)nread};
			lstr_array_add(&la, l);
		}
		line = NULL;
		len = 0;
	}
	return la;
}

void lstr_tolower(lstr l) {
	for (size_t i = 0; i < l.length; ++i) {
		l.str[i] = (char)tolower(l.str[i]);
	}
}

lstr_array tokenize(const char *str, const char *delim) {
	lstr_array la;
	lstr_array_init(&la);
	char *dstr = strdup(str);
	if (!dstr)
		goto fail;
	char *s = dstr;
	char *tok = NULL;
	while ((tok = strtok(s, delim)) != NULL) {
		char *tokstr = strdup(tok);
		if (!tokstr)
			goto fail;
		size_t len = strlen(tokstr);
		lstr l = {tokstr, len};
		lstr_tolower(l);
		lstr_array_add(&la, l);
		s = NULL;
	}
fail:
	free(dstr);
	return la;
}

int fuzzy_match(const char *str, const char *input) {
	if (!input || strlen(input) == 0) { // match all when empty
		return 1;
	}
	lstr_array tokens = tokenize(input, " ");
	lstr line_lstr = {strdup(str), strlen(str)};
	int ret = 1;
	if (!line_lstr.str)
		goto fail;
	lstr_tolower(line_lstr);
	for (size_t i = 0; i < tokens.length; ++i) {
		if (!strstr(line_lstr.str, tokens.lines[i].str)) {
			ret = 0;
			break;
		}
	}
fail:
	free(line_lstr.str);
	lstr_array_free(&tokens);
	return ret;
}

void update_matches(const char *input, lstr_array *current_matches) {
	lstr_array new_matches;
	lstr_array_init(&new_matches);
	for (size_t i = 0; i < current_matches->length; ++i) {
		if (fuzzy_match(current_matches->lines[i].str, input)) {
			lstr_array_add(&new_matches, current_matches->lines[i]);
		}
	}
	free(current_matches->lines);
	*current_matches = new_matches;
}

void print_matches(const char *input, lstr_array *current_matches, size_t choice, size_t view_offset, size_t max_lines, size_t max_cols) {
	size_t input_len = strlen(input);
	move(0, 0);
	printw("%s\n\n", input);
	size_t i = view_offset;
	size_t counter = 0;
	for (; i < current_matches->length && counter < max_lines; ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n", max_cols, current_matches->lines[i]);
		if (i == choice)
			attroff(A_REVERSE);
	}
	/* for (; i < max_lines; ++i) { */
	/* 	printw("\n"); */
	/* } */
	move(0, (int)input_len);
}

void update_choice(size_t *choice, size_t *view_offset, const lstr_array *current_matches, size_t max_lines) {
	size_t max_choice = current_matches->length - 1;
	if (*choice > max_choice)
		*choice = max_choice;
	if (*choice == *view_offset + max_lines) {
		(*view_offset)++;
	}
}

int main() {
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

	lstr_array input_lines = read_stdin_lines();
	lstr_array current_matches;
	lstr_array_init(&current_matches);
	for (size_t i = 0; i < input_lines.length; ++i) {
		lstr_array_add(&current_matches, input_lines.lines[i]);
	}
	char *output = NULL;

	size_t MAX_LINES = (size_t)LINES - 3;
	size_t MAX_COLS = (size_t)COLS - 1;
	char input[1024] = {0};
	int input_len = 0;
	int c;
	size_t choice = 0;
	size_t view_offset = 0;
	print_matches(input, &current_matches, choice, view_offset, MAX_LINES, MAX_COLS);
	while ((c = getch()) != EOF) {
		if (c == KEY_BACKSPACE || c == 0x7F) { //backspace
			input[--input_len] = '\0';
			if (input_len < 0)
				input_len = 0;
			free(current_matches.lines);
			lstr_array_init(&current_matches);
			for (size_t i = 0; i < input_lines.length; ++i) {
				lstr_array_add(&current_matches, input_lines.lines[i]);
			}
			update_matches(input, &current_matches);
			clear();
		} else if (c == KEY_ENTER || c == 0xA) {
			output = strdup(current_matches.lines[choice].str);
			break;
		} else if (c == KEY_EXIT || c == 0x1B) {
			break;
		} else if (c == KEY_UP) {
			if (choice != 0)
				choice--;
			if (choice < view_offset)
				view_offset--;
		} else if (c == KEY_DOWN) {
			choice++;
		} else if (c == KEY_RESIZE) {
			endwin();
			refresh();
			clear();
			MAX_LINES = (size_t)LINES - 3;
			MAX_COLS = (size_t)COLS - 1;
		} else {
			if (isprint(c)) {
				input[input_len++] = (char)tolower(c);
				update_matches(input, &current_matches);
				clear();
			} else {
				clear();
				printw("unknown %#x\n", c);
				getch();
				getch();
				continue;
			}
		}
		update_choice(&choice, &view_offset, &current_matches, MAX_LINES);
		print_matches(input, &current_matches, choice, view_offset, MAX_LINES, MAX_COLS);
		if (current_matches.length == 1) {
			output = strdup(current_matches.lines[0].str);
			break;
		}
	}
	lstr_array_free(&input_lines);
	endwin();
	delscreen(screen);

	if (output) {
		fflush(stdout);
		printf("%s\n", output);
		free(output);
		return 0;
	} else {
		return 1;
	}
}
