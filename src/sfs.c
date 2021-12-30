#include "str_array.h"
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

str_array read_stdin_lines()
{
	str_array la;
	str_array_init(&la);
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, stdin)) > 0) {
		if (line[nread - 1] == '\n')
			line[nread - 1] = '\0';
		str_array_add(&la, line);
		line = NULL;
		len = 0;
	}
	free(line);
	return la;
}

str_array tokenize(const char *str, const char *delim)
{
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
		str_array_add(&la, tokstr);
		s = NULL;
	}
fail:
	free(dstr);
	return la;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

size_t
lev_dist(const char *a, const size_t lena, const char *b, const size_t lenb)
{
	size_t *v0 = calloc(lenb + 1, sizeof(*v0));
	size_t *v1 = calloc(lenb + 1, sizeof(*v1));
	for (size_t i = 0; i < lena; ++i) {
		v1[0] = i + 1;
		for (size_t j = 0; j < lenb; ++j) {
			size_t deletionCost = v0[j + 1] + 1;
			size_t insertionCost = v1[j] + 1;
			size_t substitutionCost = v0[j];
			if (a[i] != b[j])
				substitutionCost++;
			v1[j + 1] = MIN(MIN(deletionCost, insertionCost),
					substitutionCost);
		}
		size_t *tmp = v0;
		v0 = v1;
		v1 = tmp;
	}
	size_t dist = v0[lenb];
	free(v0);
	free(v1);
	return dist;
}

int fuzzy_match(const char *str, unsigned str_len, str_array *tokens)
{
	int ret = 1;
	for (size_t i = 0; i < tokens->length; ++i) {
		unsigned any_good = 0;
		char *token = tokens->lower_lines[i];
		const char *s = str;
		while ((s = strstr(s, token))) {
			size_t found = (size_t)(s - str);
			if (found == 0 || !isalpha(str[found - 1]) ||
			    found + tokens->lengths[i] == str_len ||
			    !isalpha(str[found + tokens->lengths[i]])) {
				any_good = 1;
				break;
			}
			s++;
		}
		if (!any_good) {
			ret = 0;
			break;
		}
	}
	return ret;
}

void swap_str_array(str_array *arr, size_t a, size_t b)
{
	char *tmp_line = arr->lines[a];
	char *tmp_lower = arr->lower_lines[a];
	unsigned tmp_len = arr->lengths[a];

	arr->lines[a] = arr->lines[b];
	arr->lower_lines[a] = arr->lower_lines[b];
	arr->lengths[a] = arr->lengths[b];

	arr->lines[b] = tmp_line;
	arr->lower_lines[b] = tmp_lower;
	arr->lengths[b] = tmp_len;
}

size_t str_array_sort_partition(str_array *arr,
				size_t low,
				size_t high,
				size_t *distances)
{
	size_t pivot = high;
	size_t i = low;
	for (size_t j = low; j < high; ++j) {
		if (distances[j] < distances[pivot] ||
		    (distances[j] == distances[pivot] &&
		     strcmp(arr->lower_lines[j], arr->lower_lines[pivot]) <
			 0)) {
			swap_str_array(arr, i, j);
			i++;
		}
	}
	swap_str_array(arr, i, high);
	return i;
}

void str_array_quicksort(str_array *arr,
			 size_t low,
			 size_t high,
			 size_t *distances)
{
	if (low < high) {
		size_t pi = str_array_sort_partition(arr, low, high, distances);
		if (pi != 0)
			str_array_quicksort(arr, low, pi - 1, distances);
		str_array_quicksort(arr, pi + 1, high, distances);
	}
}

str_array matches(const char *input, str_array *lines)
{
	str_array matches;
	str_array_init(&matches);
	str_array tokens = tokenize(input, " ");
	for (size_t i = 0; i < lines->length; ++i) {
		if (fuzzy_match(lines->lower_lines[i],
				lines->lengths[i],
				&tokens)) {
			str_array_add(&matches, strdup(lines->lines[i]));
		}
	}
	size_t *distances = calloc(matches.length, sizeof(*distances));
	for (size_t i = 0; i < matches.length; ++i) {
		size_t total = 0;
		for (size_t j = 0; j < tokens.length; ++j) {
			total += lev_dist(matches.lower_lines[i],
					  matches.lengths[i],
					  tokens.lower_lines[j],
					  tokens.lengths[j]);
		}
		distances[i] = total;
	}
	if (matches.length > 1)
		str_array_quicksort(&matches, 0, matches.length - 1, distances);
	free(distances);
	str_array_free(&tokens);
	return matches;
}

void print_matches(const char *input,
		   str_array *current_matches,
		   size_t choice,
		   size_t view_offset,
		   const char *prompt,
		   size_t max_lines,
		   size_t max_cols)
{
	move(0, 0);
	printw("%s%s\n", prompt, input);
	size_t i = view_offset;
	size_t counter = 0;
	for (; i < current_matches->length && counter < max_lines;
	     ++i, ++counter) {
		if (i == choice)
			attron(A_REVERSE);
		printw("%.*s\n", (int)max_cols, current_matches->lines[i]);
		if (i == choice)
			attroff(A_REVERSE);
	}
	move(0, (int)(strlen(input) + strlen(prompt)));
}

void update_choice(ssize_t diff,
		   size_t *choice,
		   size_t *view_offset,
		   const str_array *current_matches,
		   size_t max_lines)
{
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

int main(int argc, char *argv[])
{
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
			fprintf(stderr,
				"usage: %s [-1] [-p prompt]\n",
				argv[0]);
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
	str_array current_matches = matches("", &input_lines);
	char *output = NULL;

	size_t MAX_LINES = (size_t)LINES - 2;
	size_t MAX_COLS = (size_t)COLS - 1;
	char input[1024] = {0};
	int input_len = 0;
	int c;
	size_t choice = 0;
	size_t view_offset = 0;
	print_matches(input,
		      &current_matches,
		      choice,
		      view_offset,
		      prompt,
		      MAX_LINES,
		      MAX_COLS);
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
			str_array_free(&current_matches);
			current_matches = matches(input, &input_lines);
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
				str_array_free(&current_matches);
				current_matches = matches(input, &input_lines);
				clear();
			} else {
				clear();
				printw("unknown %d %#x\n", c, c);
				getch();
			}
		}
		if (should_break)
			break;
		update_choice(choice_diff,
			      &choice,
			      &view_offset,
			      &current_matches,
			      MAX_LINES);
		print_matches(input,
			      &current_matches,
			      choice,
			      view_offset,
			      prompt,
			      MAX_LINES,
			      MAX_COLS);
		if (select_only_match && current_matches.length == 1) {
			output = strdup(current_matches.lines[0]);
			break;
		}
	}
	str_array_free(&current_matches);
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
