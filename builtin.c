#include "builtin.h"

// returns true if the 'exit' call
// should be performed
int exit_shell(char* cmd) {

	if (strcmp(cmd, "exit") == 0)
		return 1;

	return 0;
}

// returns true if "chdir" was performed
int cd(char* cmd) {

	char* dir;

	if (cmd[0] == 'c' && cmd[1] == 'd' &&
		(cmd[2] == ' ' || cmd[2] == END_STRING)) {

		if (cmd[2] == END_STRING) {
			// change to HOME			
			dir = getenv("HOME");
		
		} else if (cmd[2] == ' ' && cmd[3] == '$') {
			// expand variable and change to it
			dir = getenv(cmd + 4);

		} else {
			// change to the arg especified in 'cd'
			dir = cmd + 3;
		}

		if (chdir(dir) < 0) {
			fprintf(stderr, "cannot cd to %s\n", dir);
			perror(NULL);
		} else {
			memset(promt, 0, PRMTLEN);
			strcat(promt, "(");
			char* cwd = getcwd(NULL, 0);
			strcat(promt, cwd);
			free(cwd);
			strcat(promt, ")");
			status = 0;
		}

		return 1;
	}

	return 0;
}

