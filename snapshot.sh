#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
#  BTRFS-snapshot/snapshot.sh
#
#  Copyright (C) 2023 Marko Petrović <petrovicmarko2006@gmail.com>

# This is the first version of the code, written in bash.
# C++ code is based on this, and further extended.
: << COMMENT
Imagine a situation where:
$2 = /opt/VM			- the initial source for snapshot
$3 = /opt/VM1			- the initial destination of snapshot
$string = /root/opt/VM/nesto	- the subvolume nested inside /opt/VM
               ^^^^^^^
Remove everything to the $2 from the $string to get "/nesto/test" and then
prepend $3 to that to get "/opt/VM1/nesto/test" as the final destination path.
COMMENT

if [[ $1 != "create" && $1 != "delete" ]]; then
	echo "Wrong option!" >&2
	echo "Usage: snapshot create/delete <source> [dest]" >&2
	exit 1
fi

if [[ $1 == "create" && ( $2 == "" || $3 == "" ) ]]; then
	echo "Wrong path!" >&2
	echo "Usage: snapshot create/delete <source> [dest]" >&2
	exit 1
fi

if [[ $1 == "delete" && $2 == "" ]]; then
	echo "Wrong path!" >&2
	echo "Usage: snapshot create/delete <source> [dest]" >&2
	exit 1
fi

if [[ $(btrfs subvolume show "$2") == "" ]]; then
	echo "Please provide valid btrfs subvolume as source and run as root" >&2
	exit 2
fi

# Traverse filesystem and form a list of subvolumes bellow source
list="$2"
list+=$'\n'
subvol_name=$(basename "$2")
physical_path="$2"

function traverse {
	temp_list=$(btrfs subvolume list -o "$1" | cut -d " " -f 9-)
	for string in $temp_list; do
		# Remove everything to the subvolume name reported by btrfs and then prepend physical path to that subvolume
		string="${string#*$subvol_name}"
		string="$physical_path$string"
		list+="$string"
               	list+=$'\n'
               	# Check are there more subvolumes under each reported subvolume
		traverse "$string"
	done
}
traverse $2
# Remove last added \n
list=${list::-1}

if [[ $1 == "delete" ]]; then
	reverse_list=$(echo "$list" | tac)
	for string in $reverse_list; do
		finalPath=${string#*$2}
		finalPath="$2$finalPath"
		btrfs subvolume delete "$finalPath"
	done
fi
if [[ $1 == "create" ]]; then
	for string in $list; do
		# See COMMENT
		finalDest=${string#*$2}
		finalSrc="$2$finalDest"
		finalDest="$3$finalDest"
		rmdir "$finalDest" 2> /dev/null
		btrfs subvolume snapshot "$finalSrc" "$finalDest"
	done
fi
