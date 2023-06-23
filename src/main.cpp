// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/main.cpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#include <vector>
#include <string>
#include <main.hpp>

using namespace std;

char** environment;
int main(int argc, char* argv[], char* envp[])
{
	vector<string> list;
	struct program_arguments parg = parse_args(argc, argv);
	environment = envp;
	list.push_back(parg.src);

	traverse(parg.src, list, parg.src);

	if (parg.action == SNAPSHOT_DELETE)
		do_delete(list);

	if (parg.action == SNAPSHOT_CREATE)
		do_snapshot(list, parg.dest, parg.preserveFlags);

	return 0;
}
