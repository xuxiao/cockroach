#include <cstdio>
using namespace std;

#include <stdarg.h>
#include <unistd.h>
#include "utils.h"

vector<string> utils::split(const char *line, const char separator)
{
	vector<string> tokens;

	// seek to the non-space charactor
	string str;
	for (const char *ptr = line; *ptr != '\0'; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			break;
		if (*ptr == separator) {
			if (str.size() > 0) {
				tokens.push_back(str);
				str.erase();
			} else {
				; // just ignore
			}
		} else {
			str += *ptr;
		}
	}
	if (str.size() > 0)
		tokens.push_back(str);

	return tokens;
}

string utils::get_basename(const char *path)
{
	string str(path);
	return get_basename(str);
}

string utils::get_basename(string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == string::npos)
		return path;
	return path.substr(pos+1);
}

bool utils::is_absolute_path(const char *path)
{
	if (!path)
		return false;
	return (path[0] == '/');
}

bool utils::read_one_line_loop(const char *path,
                               one_line_parser_t parser, void *arg)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		return false;
	}

	static const int MAX_CHARS_ONE_LINE = 4096;
	char line[MAX_CHARS_ONE_LINE];
	while (true) {
		char *ret = fgets(line, MAX_CHARS_ONE_LINE, fp);
		if (!ret)
			break;
		(*parser)(line, arg);
	}
	fclose(fp);
	return true;
}

bool utils::is_hex_number(const char *word)
{
	for (const char *ptr = word; *ptr; ptr++) {
		if (*ptr < '0')
			return false;
		if (*ptr > 'f')
			return false;
		if (*ptr > '9' && *ptr < 'a')
			return false;
	}
	return true;
}

int utils::get_page_size(void)
{
	static int s_page_size = 0;
	if (!s_page_size)
		s_page_size = sysconf(_SC_PAGESIZE);
	return s_page_size;
}

void utils::message(const char *file, int line, const char *header,
                    const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt); 
	vprintf(fmt, ap);
	va_end(ap);
}

unsigned long
utils::calc_func_distance(void (*func0)(void), void (*func1)(void))
{
	return (unsigned long)func1 - (unsigned long)func0;
}
