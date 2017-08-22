#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define END_STRING '\0'
#define END_LINE '\n'
#define BUFLEN 1024
#define PRMTLEN 1024

static char buffer[BUFLEN];
static char promt[PRMTLEN];

static char* readline(const char* promt) {

	fprintf(stdout, "%s\n", promt);
	fprintf(stdout, "%s", "$ ");

	memset(buffer, 0, BUFLEN);

	int i = 0;
	char c = getchar();

	while (c != END_LINE && c != EOF) {

		buffer[i++] = c;

		c = getchar();
	}

	if (c == END_LINE)
		buffer[i++] = END_LINE;

	buffer[i] = END_STRING;

	return (buffer[0] != 0) ? buffer : NULL;
}

static struct cmd* parsecmd(char* buf) {
	
	return NULL;
}

static void runcmd(struct cmd* cmd) {

}

int main(int argc, char const *argv[]) {

	pid_t p;	
	char* cmd;

	char* home = getenv("HOME");

	if (chdir(home) < 0)
		fprintf(stderr, "cannot cd to %s. error: %s\n", home, strerror(errno));
	else {
		strcat(promt, "(");
		strcat(promt, home);
		strcat(promt, ")");
	}

	while ((cmd = readline(promt)) != NULL) {
		
		// if the "enter" key is pressed just
		// print the promt again
		if (cmd[0] == END_LINE)
			continue;

		cmd[strlen(cmd) - 1] = END_STRING; // chop \n

		// chdir has to be called within the father, not the child.
		if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
			
			if (chdir(cmd + 3) < 0)
				fprintf(stderr, "cannot cd to %s. error: %s\n", cmd + 3, strerror(errno));
			else {
				memset(promt, 0, PRMTLEN);
				strcat(promt, "(");
				char* cwd = get_current_dir_name();
				strcat(promt, cwd);
				free(cwd);
				strcat(promt, ")");
			}
			
			continue;
		}

		// the exit command must be called in the father
		if (strcmp(cmd, "exit") == 0)
			_exit(EXIT_SUCCESS);
/*
		if ((p = fork()) == 0)
			runcmd(parsecmd(cmd));
*/
		if ((p = fork()) == 0)
			execlp(cmd, cmd, (char*) NULL);

		waitpid(p, NULL, 0);
	}
	
	return 0;
}
