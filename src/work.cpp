// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  BTRFS-snapshot/src/work.cpp
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

static inline string substitutePaths(const string &srcPath, 
			    const string &srcPrefix, const string &destPrefix)
{
	string destPath = srcPath;
	string::iterator start = destPath.begin();

	return destPath.replace(start, start+srcPrefix.size(), destPrefix);
}

static void setFlags(const vector<string> &list, const string &dest)
{
	int srcfd, destfd;
	uint64_t flags;
	string destPath;

	for (int i = 0; i < list.size(); i++) {			
		srcfd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags);

		if ((flags & BTRFS_SUBVOL_RDONLY) == BTRFS_SUBVOL_RDONLY) {
			destPath = substitutePaths(list[i], list[0], dest);
			destfd = Open(destPath.c_str(), O_RDONLY | O_DIRECTORY);
			flags = BTRFS_SUBVOL_RDONLY;
			Ioctl(destfd, BTRFS_IOC_SUBVOL_SETFLAGS, &flags);
			close(destfd);
		}
		close(srcfd);
	}
}

void do_snapshot(const vector<string> &list, const struct program_arguments *parg)
{
	string destPath, subvolName;
	struct btrfs_ioctl_vol_args_v2 args = { 0 };
	int destfd, srcfd;

	for (int i = 0; i < list.size(); i++) {
		destPath = substitutePaths(list[i], list[0], parg->dest);
		/*
		 * Remove empty folder in place of the subvolume list[i] that
		 * was created during creation of the subvolume list[i-1].
		 */
		if (i != 0)
			check( rmdir(destPath.c_str()) )

		/* Separate subvolume name and path to that subvolume */
		subvolName = basename(destPath);
		destPath = destPath.substr(0,destPath.size()-subvolName.size());
		if (destPath == "")
			destPath = ".";

		destfd = Open(destPath.c_str(), O_RDONLY | O_DIRECTORY);
		strncpy(args.name, subvolName.c_str(), BTRFS_SUBVOL_NAME_MAX);
		args.fd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);

		Ioctl(destfd, BTRFS_IOC_SNAP_CREATE_V2, &args);
		close(destfd);
		close(args.fd);

		destPath = (destPath == ".") ? "" : destPath;
		cout << list[i] << " -> " << destPath << subvolName << endl;
	}
	if (parg->flags & OPT_PRESERVE_FLAGS)
		setFlags(list, parg->dest);
}

static vector<string> find_RO_subvolumes(const vector<string> &list,
							bool forceDelete)
{
	int srcfd;
	uint64_t flags;
	char confirm;
	vector<string> rdonly_subvols;

	for (int i = 0; i < list.size(); i++) {			
		srcfd = Open(list[i].c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags);

		if ((flags & BTRFS_SUBVOL_RDONLY) == BTRFS_SUBVOL_RDONLY) {
			if (forceDelete) {
				rdonly_subvols.push_back(list[i]);
			}
			else {
				cout << "Subvolume " << list[i] << " is marked as read-only." << endl;
				cout << "Are you sure you want to delete it? [y/N]  ";
				cin >> confirm;
				if (confirm == 'y' || confirm == 'Y')
					rdonly_subvols.push_back(list[i]);
				else
					exit(0);
			}
		}
		close(srcfd);
	}

	return rdonly_subvols;
}

void do_delete(const vector<string> &list, const struct program_arguments *parg)
{
	uint64_t flags = 0;
	int srcfd;
	struct btrfs_ioctl_vol_args_v2 args = { 0 };
	string subvolPath, subvolName;
	vector<string> rdonly_subvols = find_RO_subvolumes(list,
					       parg->flags & OPT_FORCE_DELETE);

	for (int i = 0; i < rdonly_subvols.size(); i++) {
		srcfd = Open(rdonly_subvols[i].c_str(), O_RDONLY | O_DIRECTORY);
		Ioctl(srcfd, BTRFS_IOC_SUBVOL_SETFLAGS, &flags);
		close(srcfd);
	}

	/* Remove subvolumes starting from the deepest one. */
	for (int i = list.size() - 1; i >= 0; i--) {
		/* Separate subvolume name and path to that subvolume */
		subvolName = basename(list[i]);
		subvolPath = list[i].substr(0,list[i].size()-subvolName.size());
		if (subvolPath == "")
			subvolPath = ".";

		strncpy(args.name, subvolName.c_str(), BTRFS_SUBVOL_NAME_MAX);
		srcfd = Open(subvolPath.c_str(), O_RDONLY | O_DIRECTORY);

		Ioctl(srcfd, BTRFS_IOC_SNAP_DESTROY_V2, &args);
		cout << list[i] << " deleted." << endl;
		close(srcfd);
	}
}
