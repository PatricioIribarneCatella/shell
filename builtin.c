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
	char* cwd, *old_cwd;
	char buf[BUFLEN] = {0};

	if (cmd[0] == 'c' && cmd[1] == 'd' &&
		(cmd[2] == ' ' || cmd[2] == END_STRING)) {

		if (cmd[2] == END_STRING) {
			// change to HOME			
			dir = getenv("HOME");
		
		} else if (cmd[2] == ' ' && cmd[3] == '$') {
			// expand variable and change to it
			dir = getenv(cmd + 4);

		} else if (cmd[2] == ' ' && cmd[3] == '-') {
			// change to the previous cwd
			dir = getenv("OLDPWD");

		} else {
			// change to the arg especified in 'cd'
			dir = cmd + 3;
		}

		old_cwd = getcwd(NULL, 0);

		if (chdir(dir) < 0) {
			snprintf(buf, sizeof buf, "cannot cd to %s ", dir);
			perror(buf);
		} else {
			memset(promt, 0, PRMTLEN);
			cwd = getcwd(NULL, 0);

			snprintf(promt, sizeof promt, "(%s)", cwd);
			
			setenv("PWD", cwd, 1);
			setenv("OLDPWD", old_cwd, 1);
			
			free(cwd);
			status = 0;
		}

		free(old_cwd);

		return 1;
	}

	return 0;
}

// returns true if 'pwd' was executed
int pwd(char* cmd) {

	char buf[BUFLEN];

	if (strcmp(cmd, "pwd") == 0) {
		
		getcwd(buf, sizeof buf);
		
		printf("%s\n", buf);
		
		status = 0;

		return 1;
	}

	return 0;

}

// returns true if export was performed
int export_var(char* cmd) {
	
	int equalIdx;
	char key[100], value[100], var[BUFLEN];

	// if no parameter was passed to the 'export' call
	if (strcmp(cmd, "export") == 0 ||
	    strcmp(cmd, "export ") == 0) {

		fprintf(stderr, "missing parameter: KEY=VALUE not faund\n");
		status = 1;
		return 1;
	}

	if (sscanf(cmd, "export %s", var) > 0) {

		equalIdx = block_contains(var, '=');

		// if the parameter passed does not contain the '='
		if (equalIdx < 0) {
			fprintf(stderr, "invalid variable arg to set\n");
			status = 1;
			return 1;
		}
		
		get_environ_key(var, key);
		get_environ_value(var, value, equalIdx);

		setenv(key, value, 1);

		status = 0;

		return 1;
	}

	return 0;
}

