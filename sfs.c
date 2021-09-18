#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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
	size_t nread;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		if (nread != 0 && line[nread-1] == '\n') {
			line[nread-1] = '\0';
			nread--;
		}
		if (nread != 0) {
			lstr l = {line, nread};
			lstr_array_add(&la, l);
		}
		line = NULL;
		len = 0;
	}
	return la;
}

void lstr_tolower(lstr l) {
	for (size_t i = 0; i < l.length; ++i) {
		l.str[i] = tolower(l.str[i]);
	}
}

lstr_array tokenize(const char *str, const char *delim) {
	lstr_array la;
	lstr_array_init(&la);
	char *dstr = strdup(str);
	char *s = dstr;
	char *tok = NULL;
	while ((tok = strtok(s, delim)) != NULL) {
		char *tokstr = strdup(tok);
		size_t len = strlen(tokstr);
		lstr l = {tokstr, len};
		lstr_tolower(l);
		lstr_array_add(&la, l);
		s = NULL;
	}
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
	lstr_tolower(line_lstr);
	for (size_t i = 0; i < tokens.length; ++i) {
		if (!strstr(line_lstr.str, tokens.lines[i].str)) {
			ret = 0;
			break;
		}
	}
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

void print_matches(const char *input, lstr_array *current_matches, int choice, int max_lines) {
	size_t input_len = strlen(input);
	move(0, 0);
	printw("%s\n\n", input);
	size_t i = 0;
	for (; i < current_matches->length && i < max_lines; ++i) {
		if (i == choice)
			attron(A_REVERSE);
		if (i < max_lines)
			printw("%s\n", current_matches->lines[i]);
		if (i == choice)
			attroff(A_REVERSE);
	}
	/* for (; i < max_lines; ++i) { */
	/* 	printw("\n"); */
	/* } */
	move(0, input_len);
}

int update_choice(int choice, const lstr_array *current_matches, int max_lines) {
	if (choice < 0) {
		choice = 0;
		return choice;
	}
	int bound = MIN(current_matches->length - 1, max_lines - 1);
	choice = MIN(bound, choice);
	return choice;
}

int main() {
	FILE *tty = fopen("/dev/tty", "rw");
	SCREEN *screen = newterm(NULL, stdout, tty);
	set_term(screen);
	noecho();
	cbreak();
	scrollok(stdscr, TRUE);
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);

	lstr_array input_lines = read_stdin_lines();
	lstr_array current_matches;
	lstr_array_init(&current_matches);
	for (size_t i = 0; i < input_lines.length; ++i) {
		lstr_array_add(&current_matches, input_lines.lines[i]);
	}
	char *output = NULL;

	const int MAX_LINES = LINES - 3;

	char input[1024] = {0};
	int input_len = 0;
	int c;
	int choice = 0;
	print_matches(input, &current_matches, choice, MAX_LINES);
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
		} else if (c == KEY_UP) {
			choice--;
		} else if (c == KEY_DOWN) {
			choice++;
		} else {
			if (isprint(c)) {
				input[input_len++] = tolower(c);
				update_matches(input, &current_matches);
				clear();
			} else {
				clear();
				printw("unknown %#x\n", c);
				getch();
				continue;
			}
		}
		choice = update_choice(choice, &current_matches, MAX_LINES);
		print_matches(input, &current_matches, choice, MAX_LINES);
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
	}
}
