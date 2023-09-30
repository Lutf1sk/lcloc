#include <lt/io.h>
#include <lt/debug.h>
#include <lt/arg.h>
#include <lt/str.h>
#include <lt/mem.h>
#include <lt/ansi.h>
#include <lt/darr.h>

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

lt_alloc_t* arena = NULL;

usz code_total = 0;
usz blank_total = 0;
usz comment_total = 0;

usz file_count = 0;

b8 with_color = 0;
b8 print_all = 0;

void print_line_counts(usz code, usz blank, usz comment) {
	char* code_clr = "", *blank_clr = "", *comment_clr = "", *reset = "";
	if (with_color) {
		code_clr = LT_FG_BYELLOW;
		blank_clr = LT_FG_WHITE;
		comment_clr = LT_FG_BGREEN;
		reset = LT_RESET;
	}

	lt_printf(
		"code    %s%uz%s\n"
		"blank   %s%uz%s\n"
		"comment %s%uz%s\n",
		code_clr, code, reset,
		blank_clr, blank, reset,
		comment_clr, comment, reset);
}

void increment_line_type(lstr_t line) {
	line = lt_lstr_trim_left(line);

	if (!line.len)
		++blank_total;
	else if (line.len >= 2 && line.str[0] == '/' && line.str[1] == '/')
		++comment_total;
	else
		++code_total;
}

void count_lines(lstr_t path) {
	char cpath[1024];
	cpath[lt_sprintf(cpath, "%S", path)] = 0;

	struct stat st;
	if (stat(cpath, &st) < 0) {
		lt_werrf("failed to stat '%S'\n", path);
		return;
	}

	if (S_ISDIR(st.st_mode)) {
		DIR* dir = opendir(cpath);
		if (!dir) {
			lt_werrf("failed to open directory '%S'\n", path);
			return;
		}

		struct dirent* ent;
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.')
				continue;
			while (path.str[path.len - 1] == '/')
				path.len--;
			lstr_t name = lt_lstr_from_cstr(ent->d_name);
			cpath[lt_sprintf(cpath, "%S/%S", path, name)] = 0;
			count_lines(lt_lstr_from_cstr(cpath));
		}

		closedir(dir);
		return;
	}

	if (!lt_lstr_endswith(path, CLSTR(".c")) && !lt_lstr_endswith(path, CLSTR(".h")) &&
		!lt_lstr_endswith(path, CLSTR(".cpp")) && !lt_lstr_endswith(path, CLSTR(".hpp")))
		return;

	lstr_t contents;
	if (lt_file_read_entire(path, &contents, arena))
		lt_werrf("failed to open '%S'\n", path);

	usz code_start = code_total;
	usz blank_start = blank_total;
	usz comment_start = comment_total;

	char* it = contents.str, *end = it + contents.len, *line_start = it;
	while (it < end) {
		if (*it != '\n') {
			++it;
			continue;
		}
		increment_line_type(lt_lstr_from_range(line_start, it));
		line_start = ++it;
	}
	increment_line_type(lt_lstr_from_range(line_start, it));

	if (print_all) {
		lt_printf("%S:\n", path);
		print_line_counts(code_total - code_start, blank_total - blank_start, comment_total - comment_start);
		lt_printf("\n");
	}

	++file_count;

	lt_mfree(arena, contents.str);
}

int main(int argc, char** argv) {
	LT_DEBUG_INIT();

	arena = (lt_alloc_t*)lt_amcreate(NULL, LT_GB(1), 0);

	lt_darr(char*) path_list = lt_darr_create(char*, argc, arena);

	lt_arg_iterator_t arg_it = lt_arg_iterator_create(argc, argv);
	while (lt_arg_next(&arg_it)) {
		if (lt_arg_flag(&arg_it, 'h', CLSTR("help"))) {
			lt_printf(
				"usage: lcloc [OPTIONS] FILES\n"
				"options:\n"
				"  -h, --help           Display this information.\n"
				"  -c, --color          Display output in multiple colors.\n"
				"  -a, --all            Print the lines of each individual file.\n"
			);
			return 0;
		}

		if (lt_arg_flag(&arg_it, 'c', CLSTR("color"))) {
			with_color = 1;
			continue;
		}

		else if (lt_arg_flag(&arg_it, 'a', CLSTR("all"))) {
			print_all = 1;
			continue;
		}

		lt_darr_push(path_list, *arg_it.it);
	}

	for (usz i = 0; i < lt_darr_count(path_list); ++i)
		count_lines(lt_lstr_from_cstr(path_list[i]));

	lt_printf("total across %uz file(s):\n", file_count);
	print_line_counts(code_total, blank_total, comment_total);
	return 0;
}