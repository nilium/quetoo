/*
 * Copyright(c) 1997-2001 id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quetoo.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if defined(_WIN32)

#include <windows.h>
#include <shellapi.h>

/**
* @brief Find the first character of a given list of characters.
*/
static ssize_t str_find_first_of(const char *string, const char *characters) {

	for (const char *c = string; *c; c++) {

		for (const char *character = characters; *character; character++) {

			if (*c == *character) {
				return string - c;
			}
		}
	}

	return -1;
}

/**
* @brief This routine appends the given argument to a command line such
that CommandLineToArgvW will return the argument string unchanged.
Arguments in a command line should be separated by spaces; this
function does not add these spaces. Thanks @ https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
*/
static gchar *ArgvQuote(const gchar *argument, const _Bool force) {
	if (!force &&
		strlen(argument) > 0 &&
		str_find_first_of(argument, " \t\n\v\"") == -1) {
		return g_strdup(argument);
	}

	GString *commandLine = g_string_new("\"");

	for (const char *c = argument; ; ++c) {
		size_t num_slashes = 0;

		while (*c && *c == '\\') {
			c++;
			num_slashes++;
		}

		if (!*c) {

			for (size_t i = 0; i < num_slashes * 2; i++) {
				g_string_append_c(commandLine, '\\');
			}

			break;
		}
		else if (*c == '"') {

			for (size_t i = 0; i < (num_slashes * 2) + 1; i++) {
				g_string_append_c(commandLine, '\\');
			}
		}
		else {

			for (size_t i = 0; i < num_slashes; i++) {
				g_string_append_c(commandLine, '\\');
			}
		}

		g_string_append_c(commandLine, *c);
	}

	g_string_append_c(commandLine, '"');
	return g_string_free(commandLine, false);
}

/**
* @brief Windows must use CreateProcess because _spawn/_exec on Windows keeps the calling process
* running, which is definitely not what we want.
*/
static intptr_t execvp(const char *filename, char *const * args) {
	GString *cmdline = g_string_new("");

	for (int i = 1; ; i++) {

		if (!args[i]) {
			break;
		} else if (i != 1) {
			g_string_append_c(cmdline, ' ');
		}

		gchar *arg = ArgvQuote(args[i], false);
		g_string_append(cmdline, arg);
		g_free(arg);
	}

	SHELLEXECUTEINFO execInfo;
	memset(&execInfo, 0, sizeof(SHELLEXECUTEINFO));
	execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	execInfo.lpFile = filename;
	execInfo.lpParameters = cmdline->str;
	execInfo.nShow = SW_HIDE;

	if (!ShellExecuteEx(&execInfo)) {
		g_string_free(cmdline, true);
		return GetLastError();
	}

	g_string_free(cmdline, true);
	return 0;
}

int main(int argc, char **argv);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}

#ifndef realpath
	#define realpath(rel, abs) _fullpath(abs, rel, MAX_PATH)
#endif

#endif
