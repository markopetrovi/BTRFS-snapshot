// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/execute.cpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#include <main.hpp>
#include <cstdarg>
#include <unistd.h>
#include <string>

using namespace std;

static char** argv_create(int argc, va_list *ptr)
{
	char** argv = new char*[argc+1];

	for(int i = 0; i < argc; i++)
		argv[i] = va_arg(*ptr, char*);
	argv[argc] = NULL;

	return argv;
}

int execute(int argc, int stdin_pipe[], int stdout_pipe[], int stderr_pipe[], ...)
{
	int pid = fork();
	if (pid == 0) {
		va_list ptr;
		va_start(ptr, stderr_pipe);
		char **argv = argv_create(argc, &ptr);
		va_end(ptr);

		stderr_redirect(stderr_pipe);
		stdin_redirect(stdin_pipe);
		stdout_redirect(stdout_pipe);

		string path = string("/bin/") + string(argv[0]);
		execve(path.c_str(), argv, environment);
		/*
		 * execve(2) failed. Try with different path and print error
		 * message if that fails too.
		 */
		path = string("/sbin/") + string(argv[0]);
		Execve(path.c_str(), argv, environment);
	}
	close(write_end(stdout_pipe));
	close(write_end(stderr_pipe));
	close(read_end(stdin_pipe));
	return pid;
}
