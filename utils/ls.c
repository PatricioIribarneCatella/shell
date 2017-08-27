#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILESIZE 200

static void ls(const char* path) {

	DIR* d;
	struct stat st;
	struct dirent entry;
	struct dirent* result;
	char buf[10];
	char filepath[FILESIZE];
	int r;

	if (stat(path, &st) < 0) {
		printf("cannot stat directory/file: %s. error: %s\n",
			path, strerror_r(errno, buf, sizeof(buf)));
		return;
	}

	// It´s a regular file
	if (S_ISREG(st.st_mode)) {
		printf("mode: %lo size: %ld  uid: %d  gid: %d inode: %lu file: %s\n",
			(unsigned long)st.st_mode, st.st_size, st.st_uid, st.st_gid, st.st_ino, path);
		return;
	}
	
	// It´s a directory
	if (S_ISDIR(st.st_mode)) {

		if (!(d = opendir(path))) {
			printf("cannot open directory: %s. error: %s\n",
				path, strerror_r(errno, buf, sizeof(buf)));
			return;
		}

		r = readdir_r(d, &entry, &result);
		
		while ((r == 0) && (result)) {
			
			memset(filepath, 0, sizeof(filepath));
			strcpy(filepath, path);
			strcat(filepath, "/");
			strcat(filepath, entry.d_name);

			if (stat(filepath, &st) < 0) {
				printf("cannot stat directory/file: %s. error: %s\n",
					filepath, strerror_r(errno, buf, sizeof(buf)));
			} else {
				printf("mode: %lo size: %ld  uid: %d  gid: %d inode: %lu file: %s\n",
					(unsigned long)st.st_mode, st.st_size, st.st_uid,
					st.st_gid, st.st_ino, entry.d_name);
			}

			r = readdir_r(d, &entry, &result);
		}

		closedir(d);
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
