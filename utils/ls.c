#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static void ls(const char* path) {

	DIR* d;
	struct stat st;
	struct dirent* entry;
	int fd;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("cannot open");
		return;
	}

	if (fstat(fd, &st) < 0) {
		perror("cannot stat");
		close(fd);
		return;
	}

	// It´s a regular file
	if (S_ISREG(st.st_mode)) {
		printf("mode: %lo size: %ld  uid: %d  gid: %d inode: %lu file: %s\n",
			(unsigned long)st.st_mode, st.st_size, st.st_uid, st.st_gid, st.st_ino, path);
		close(fd);
		return;
	}
	
	// It´s a directory
	if (S_ISDIR(st.st_mode)) {

		if (!(d = fdopendir(fd))) {
			perror("cannot open dir");
			close(fd);
			return;
		}

		errno = 0;

		while ((entry = readdir(d))) {
			
			if (fstatat(fd, entry->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0) {
				perror("cannot stat");
			} else {
				printf("mode: %lo size: %ld  uid: %d  gid: %d inode: %lu file: %s\n",
					(unsigned long)st.st_mode, st.st_size, st.st_uid,
					st.st_gid, st.st_ino, entry->d_name);
			}
		}

		if (errno)
			perror("cannot readdir");

		closedir(d);
		close(fd);
	}
}

int main(int argc, char const* argv[]) {

	if (argc < 2)
		ls(".");
	else {
		for (int i = 1; i < argc; i++) {
			printf("	%s\n", argv[i]);
			ls(argv[i]);
		}
	}

	return 0;
}
