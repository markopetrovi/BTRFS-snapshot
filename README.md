# snapshot
### BTRFS recursive snapshot utility
By default, the utility from btrfs-progs doesn't snapshot subvolumes recursively. This can be useful for intentionally excluding certain subvolumes from snapshots, but it can also be bad (and unavoidable) for things like snapshotting entire system when there are nested subvolumes on it.\
This tool lists subvolumes present on the system and recursively snapshots each subvolume inside the nested subvolume tree.
### Advantages
- Fast location of nested subvolumes using btrfs ioctls
- Able to delete subvolumes containing more nested subvolumes in them
- Able to create snapshots of nested subvolume trees
- Has an option (--preserve) to copy the read-only flag from original subvolumes to snapshotted subvolumes
- Able to delete subvolume trees containing read-only subvolumes in a single run (with warnings)
- Prints a little help message if the arguments are wrong
### Disadvantages
- Still dependent on btrfs-progs for obtaining subvolume list (other things use ioctls directly)
- Requires root privileges to search filesystem
- No option to exclude certain subvolumes from snapshotting, yet
- Generation of snapshots containing more than one subvolume is not atomic (this is currently impossible to fix in userspace)
### Compilation
There are no special build dependencies. Just download the source code and run make & sudo make install and the utility will be installed to /bin/snapshot
## Disclaimer
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License 2.0 for more details.

Use this program at your own risk. I do not hold responsibility for any damage (including data loss) that may be caused while using (correctly or incorrectly) this program.
