#include "matcher.hpp"
#include "util.hpp"
#include <algorithm>
#include <cctype>

Matcher::Matcher(const std::vector<std::string> &lines)
    : lines(lines), lower_lines(lines_to_lower(lines)) {
	reset_matches();
}

std::vector<std::string>
Matcher::lines_to_lower(const std::vector<std::string> &lines) {
	std::vector<std::string> lower_lines;
	lower_lines.reserve(lines.size());
	std::transform(lines.begin(),
		       lines.end(),
		       std::back_inserter(lower_lines),
		       [](const auto &str) { return str_tolower(str); });
	return lower_lines;
}

bool Matcher::fuzzy_match(const std::string &str,
			  const std::vector<std::string> &tokens) {
	bool has_all_matches = true;
	for (const auto &token : tokens) {
		if (str.find(token) == std::string::npos) {
			has_all_matches = false;
			break;
		}
	}
	return has_all_matches;
}

void Matcher::update_matches() {
	std::vector<std::size_t> new_matches;
	const auto tokens = tokenize(pattern, " ");
	for (auto match_id : matches) {
		if (fuzzy_match(lower_lines[match_id], tokens)) {
			new_matches.push_back(match_id);
		}
	}
	std::swap(new_matches, matches);
}

const std::string &Matcher::get_pattern() const {
	return pattern;
}

const std::vector<std::size_t> &Matcher::get_matches() const {
	return matches;
}

void Matcher::pop() {
	if (pattern.length() > 0)
		pattern.pop_back();
	reset_matches();
	update_matches();
}

void Matcher::push(char c) {
	pattern.push_back(static_cast<char>(std::tolower(c)));
	update_matches();
}

void Matcher::reset_matches() {
	matches.clear();
	matches.reserve(lines.size());
	for (std::size_t i = 0; i < lines.size(); ++i) {
		matches.push_back(i);
	}
}
