// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/include/main.hpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#ifndef CHECK_H
#define CHECK_H
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <sys/wait.h>
#include <string>
#include <vector>

using namespace std;

#define check(function)								\
	if((function) < 0)							\
	{									\
		fprintf(stderr, #function);					\
		fprintf(stderr, " Failed: %i\n%m\n", errno);			\
		exit(1);							\
	}
#define read_end(pipefd)	(pipefd) ? pipefd[0] : -1
#define write_end(pipefd)	(pipefd) ? pipefd[1] : -1

#define SNAPSHOT_CREATE		1
#define SNAPSHOT_DELETE		2

struct program_arguments {
	string src, dest;
	bool preserveFlags;
	int action;
};

extern char** environment;

/* execute.cpp */
extern int execute(int argc, int stdin_pipe[], int stdout_pipe[], int stderr_pipe[], ...);

/* btrfs.cpp */
extern void traverse(const string &current_subvolpath, vector<string> &list,
					     const string &initial_subvolpath);

/* helper.cpp */
extern struct program_arguments parse_args(int argc, char* argv[]);
extern string read_pipe(int pipefd);
extern string basename(const string &path);

/* work.cpp */
extern void do_snapshot(const vector<string> &list, const string &dest_path,
							   bool preserveFlags);
extern void do_delete(const vector<string> &list);

/* Inline helper functions: */
static inline void wait_for_exit(int pid)
{
	int status = 0;
	do {
		waitpid(pid, &status, 0);
	} while(!WIFEXITED(status));
}

static inline void stdin_redirect(int pipefd[])
{
	close(write_end(pipefd));
	dup2(read_end(pipefd), STDIN_FILENO);
	close(read_end(pipefd));
}

static inline void stdout_redirect(int pipefd[])
{
	close(read_end(pipefd));
	dup2(write_end(pipefd), STDOUT_FILENO);
	close(write_end(pipefd));
}

static inline void stderr_redirect(int pipefd[])
{
	close(read_end(pipefd));
	dup2(write_end(pipefd), STDERR_FILENO);
	close(write_end(pipefd));
}

/* Error-checking functions as wrappers around regular system calls: */
static inline void Pipe(int pipefd[])
{
	check( pipe(pipefd) )
}

static inline void Execve(const char *pathname, char *const argv[], char *const envp[])
{
	execve(pathname, argv, envp);
	fprintf(stderr, "Execve for %s failed!\n%m\n", pathname);
	exit(2);
}

static inline int Write(int fd, const void *buf, size_t count)
{
	int ret = write(fd, buf, count);
	if (ret < 0) {
		fprintf(stderr, "write(%i, buf, %li)", fd, count);
		fprintf(stderr, " Failed: %i\n%m\n", errno);
		exit(3);
	}
	return ret;
}

static inline int Read(int fd, void *buf, size_t count)
{
	int ret = read(fd, buf, count);
	if (ret < 0) {
		fprintf(stderr, "read(%i, buf, %li)", fd, count);
		fprintf(stderr, " Failed: %i\n%m\n", errno);
		exit(4); 
	}
	return ret;
}

static inline int Open(const char *pathname, int flags)
{
	int fd = open(pathname, flags);
	if (fd < 0) {
		fprintf(stderr, "open(%s) Failed: %i\n%m\n", pathname, errno);
		exit(5);
	}
	return fd;
}

#endif
