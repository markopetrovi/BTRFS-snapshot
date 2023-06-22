// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  snapshot/work.cpp
 *
 *  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>
 */
#include <main.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <linux/btrfs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <errno.h>

using namespace std;

static void Ioctl(int fd, unsigned long request, void *arg)
{
	struct btrfs_ioctl_vol_args_v2 *args;

	if (ioctl(fd, request, arg) >= 0)
		return;
	if ((request & BTRFS_IOC_SUBVOL_GETFLAGS) == BTRFS_IOC_SUBVOL_GETFLAGS) {
		fprintf(stderr, "ioctl(%i, BTRFS_IOC_SUBVOL_GETFLAGS, ...) Failed: %i\n%m\n", fd, errno);
		fprintf(stderr, "Cannot get flags of the given subvolume!\n");
		exit(8);
	}
	if ((request & BTRFS_IOC_SUBVOL_SETFLAGS) == BTRFS_IOC_SUBVOL_SETFLAGS) {
		fprintf(stderr, "ioctl(%i, BTRFS_IOC_SUBVOL_SETFLAGS, ...) Failed: %i\n%m\n", fd, errno);
		fprintf(stderr, "Cannot set flags for the given subvolume!\n");
		exit(8);
	}
	if ((request & BTRFS_IOC_SNAP_CREATE_V2) == BTRFS_IOC_SNAP_CREATE_V2) {
		args = (struct btrfs_ioctl_vol_args_v2 *) arg;
		fprintf(stderr, "ioctl(%i, BTRFS_IOC_SNAP_CREATE_V2, {fd=%i, flags=%llu, name=\"%s\"}) Failed: %i\n%m\n", fd, args->fd, args->flags, args->name, errno);
		fprintf(stderr, "Cannot create snapshot \"%s\"\n", args->name);
		exit(9);
	}
	fprintf(stderr, "Unknown ioctl request failed: %i\n%m\n", errno);
	exit(10);
}
void do_snapshot(const vector<string> &list, const string &dest_path, 
							bool preserveFlags)
{
	string dest_subvol, subvol_name;
	string::iterator start;
	struct btrfs_ioctl_vol_args_v2 args = { 0 };
	int destfd, srcfd;
	uint64_t flags;

	for (int i = 0; i < list.size(); i++) {
		dest_subvol = list[i];
		start = dest_subvol.begin();

		/*
		 * For each sub-subvolume replace path to the initial subvolume
		 * with the destination path.
		 * list[0] - initial (first to be snapshotted) subvolume (path)
		 */
		dest_subvol.replace(start, start + list[0].size(), dest_path);
		/*
		 * Remove empty folder in place of the subvolume list[i] that
		 * was created during creation of the subvolume list[i-1].
		 */
		if (i != 0)
			check( rmdir(dest_subvol.c_str()) )

		/* Separate subvolume name and path to that subvolume */
		subvol_name = basename(dest_subvol);
		dest_subvol = dest_subvol.substr(0,
				      dest_subvol.size()-subvol_name.size());
		if (dest_subvol == "")
			dest_subvol = ".";

		destfd = Open(dest_subvol.c_str(), O_RDONLY | O_DIRECTORY);
		strncpy(args.name, subvol_name.c_str(), BTRFS_SUBVOL_NAME_MAX);
		args.fd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);

		Ioctl(destfd, BTRFS_IOC_SNAP_CREATE_V2, &args);
		close(destfd);
		close(args.fd);
		dest_subvol = (dest_subvol == ".") ? "" : dest_subvol;
		cout << list[i] << " -> " << dest_subvol << subvol_name << endl;
	}
	if (preserveFlags)
		for (int i = 0; i < list.size(); i++) {			
			srcfd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);
			Ioctl(srcfd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags);
			if ((flags & BTRFS_SUBVOL_RDONLY) == BTRFS_SUBVOL_RDONLY) {
				dest_subvol = list[i];
				start = dest_subvol.begin();
				dest_subvol.replace(start, start+list[0].size(),
								     dest_path);
				destfd = Open(dest_subvol.c_str(), O_RDONLY |
								   O_DIRECTORY);
				flags = BTRFS_SUBVOL_RDONLY;
				Ioctl(destfd, BTRFS_IOC_SUBVOL_SETFLAGS, &flags);
				close(destfd);
			}
			close(srcfd);
		}
}

void do_delete(const vector<string> &list)
{
	uint64_t flags;
	int srcfd;
	char confirm;
	vector<string> rdonly_subvols;
	string subvol_path, subvol_name;
	struct btrfs_ioctl_vol_args_v2 args = { 0 };

	for (int i = 0; i < list.size(); i++) {			
		srcfd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags);
		if ((flags & BTRFS_SUBVOL_RDONLY) == BTRFS_SUBVOL_RDONLY) {
			cout << "Subvolume " << list[i] << " is marked as read-only." << endl;
			cout << "Are you sure you want to delete it? [y/N]  ";
			cin >> confirm;
			if (confirm == 'y' || confirm == 'Y')
				rdonly_subvols.push_back(list[i]);
			else
				exit(0);
		}
		close(srcfd);
	}
	flags = 0;
	for (int i = 0; i < rdonly_subvols.size(); i++) {
		srcfd = Open(rdonly_subvols[i].c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SUBVOL_SETFLAGS, &flags);
		close(srcfd);
	}

	/* Remove subvolumes starting from the deepest one. */
	for (int i = list.size() - 1; i >= 0; i--) {
		subvol_path = list[i];
		/* Separate subvolume name and path to that subvolume */
		subvol_name = basename(subvol_path);
		subvol_path = subvol_path.substr(0,
				      subvol_path.size()-subvol_name.size());
		if (subvol_path == "")
			subvol_path = ".";

		strncpy(args.name, subvol_name.c_str(), BTRFS_SUBVOL_NAME_MAX);
		srcfd = Open(subvol_path.c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SNAP_DESTROY_V2, &args);
		cout << list[i] << " deleted." << endl;
		close(srcfd);
	}
}
