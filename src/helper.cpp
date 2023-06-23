// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/helper.cpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#include <main.hpp>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

string basename(const string &path)
{
	size_t lastSlash = path.find_last_of("/");
	if (lastSlash != string::npos)
		return path.substr(lastSlash + 1);
	return path;
}

string read_pipe(int pipefd)
{
	string str;
	char buf[101];

	while (true) {
		int a = Read(pipefd, buf, sizeof(buf)-1);
		buf[a] = '\0';
		if (a == 0)
			break;
		str += buf;
	}
	return str;
}

struct program_arguments parse_args(int argc, char* argv[])
{
	struct program_arguments parg;
	bool noMoreOptions = 0;
	int unknownArg = 0;
	string errString = "Wrong usage!\n";
	errString += "Usage: snapshot create/delete [--preserve/-p] <source> [dest]";

	if (argc < 3) {
		cerr << errString << endl;
		exit(6);
	}

	if (string(argv[1]) == "create")
		parg.action = SNAPSHOT_CREATE;
	else if (string(argv[1]) == "delete")
		parg.action = SNAPSHOT_DELETE;
	else {
		cerr << errString << endl;
		exit(6);
	}

	parg.preserveFlags = 0;
	for (int i = 2; i < argc; i++) {
		string arg_i = string(argv[i]);
		if (arg_i == "--") {
			noMoreOptions = 1;
			continue;
		}
		if (arg_i[0] == '-' && !noMoreOptions) {
			if (arg_i == "--preserve" || arg_i == "-p") {
				parg.preserveFlags = 1;
				continue;
			}
			cerr << errString << endl;
			exit(6);
		}
		if (unknownArg == 0) {
			parg.src = arg_i;
			unknownArg++;
			continue;
		}
		if (unknownArg == 1) {
			parg.dest = arg_i;
			unknownArg++;
			continue;
		}
		cerr << errString << endl;
		exit(6);
	}
	
	if (parg.src == "" || (string(argv[1]) == "create" && parg.dest == "")) {
		cerr << errString << endl;
		exit(6);
	} 

	int stderr_pipe[2];
	int stdout_redirect[2];
	stdout_redirect[1] = Open("/dev/null", O_WRONLY);
	Pipe(stderr_pipe);

	int pid = execute(4, NULL, stdout_redirect, stderr_pipe, "btrfs",
					"subvolume", "show", parg.src.c_str());
	wait_for_exit(pid);
	string btrfs_err = read_pipe(read_end(stderr_pipe));
	close(read_end(stderr_pipe));
	if(btrfs_err != "") {
		cerr << "Please provide valid btrfs subvolume as source and run as root" << endl;
		cerr << btrfs_err << endl;
		exit(7);
	}

	return parg;
}
