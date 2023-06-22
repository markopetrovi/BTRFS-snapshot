// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/btrfs.cpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#include <unistd.h>
#include <sstream>
#include <string>
#include <main.hpp>
#include <vector>

using namespace std;

/* Append the list of all subvolumes under subvolpath to the vector list */
void traverse(const string &current_subvolpath, vector<string> &list,
						const string &initial_subvolpath)
{
	static string initial_subvolname = basename(initial_subvolpath);
	istringstream current_list;
	int pipe_connect[2];
	int stdout_pipe[2];

	Pipe(pipe_connect);
	Pipe(stdout_pipe);
	/* 
	 * List current subvolume and obtain current_list of subvolumes under it,
	 * not including the current subvolume.
	 * current_list=$(btrfs subvolume list -o "$1" | cut -d " " -f 9-)
	 */
	int pid1 = execute(5, NULL, pipe_connect, NULL, "btrfs", "subvolume",
				     "list", "-o", current_subvolpath.c_str());

	int pid2 = execute(5, pipe_connect, stdout_pipe, NULL, "cut", "-d",
							      " ", "-f", "9-");
	wait_for_exit(pid1);
	wait_for_exit(pid2);
	current_list.str(read_pipe(read_end(stdout_pipe)));
	close(read_end(stdout_pipe));

	string token;
	while(getline(current_list, token)) {
		/*
		* # 1) Remove everything to the end of the initial_subvolname.
		* # In the following situation:
		* # initial_subvolpath = "/opt/VM"
		* # initial_subvolname = "VM"
		* # current_subvolpath = "/opt/VM/nesto
		* # token = "/root/opt/VM/nesto/novi"
		*                        ^ - delete everything to here
		* ## resulting_string = "/nesto/novi"
		* 
		* 2) Prepend initial_subvolpath to that resulting_string to get
		*    path relative to the mounted root:
		* ## resulting_path (newsubvolume_path) = /opt/VM/nesto/novi
		*
		* Motive: Get rid of the leading /root in the string token.
		*/
		size_t res_str_i = token.find(initial_subvolname);
		if (res_str_i != string::npos) {
			res_str_i += initial_subvolname.size();

			string res_str = token.substr(res_str_i);
			string newsubvol_path = initial_subvolpath + res_str;
			list.push_back(newsubvol_path);

			traverse(newsubvol_path, list, initial_subvolpath);
		}
	}
}
