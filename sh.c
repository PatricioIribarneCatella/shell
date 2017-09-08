#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define COLOR_BLUE "\x1b[34m"
#define COLOR_RED "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

#define END_STRING '\0'
#define END_LINE '\n'
#define SPACE ' '
#define BUFLEN 1024
#define PRMTLEN 1024
#define MAXARGS 20
#define ARGSIZE 20

// Command representation after parsed
#define EXEC 1
#define BACK 2

struct cmd {
	int type;
};

struct execcmd {
	int type;
	char* argv[MAXARGS];
	char* eargv[MAXARGS];
};

int status;
pid_t back;
static char buffer[BUFLEN];
static char promt[PRMTLEN];
static char numbers[10] = "0123456789";
static char bufNum[32];

// SIGCHLD new handler
// If the *back* pid is not zero
// (a process has been running in background)
// then it waits for it to finish and then exists.
void handler(int snumber) {

	int s;

	if (back != 0) {
		wait(&s);
		printf("	process %d running in background done\n", back);
		_exit(EXIT_SUCCESS);
	}
}

static char* itoa(int val) {

	if (val == 0) return "0";

	int i;

	for (i = 30; val > 0 ; --i) {

		bufNum[i] = numbers[val % 10];
		val /= 10;
	}

	return &bufNum[i + 1];
}

static char* readline(const char* promt) {

	fprintf(stdout, "%s %s %s\n", COLOR_RED, promt, COLOR_RESET);
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

	struct execcmd* cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));

	cmd->type = EXEC;

	int idx, argc = 0, i = 0;

	while (buf[i] != END_STRING) {
		
		if (buf[i] == '&') {
			// run a process in background
			cmd->type = BACK;
			i++;
		} else {
			char* arg = malloc(ARGSIZE);
			memset(arg, 0, ARGSIZE);	
			idx = 0;

			while (buf[i] != SPACE && buf[i] != END_STRING) {
				arg[idx] = buf[i];
				i++; idx++;
			}

			i++; // goes to the next argument

			// expand environment variables
			if (arg[0] == '$') {

				char* aux = arg;
			
				if (arg[1] == '?')
					arg = itoa(status);
				else
					arg = getenv(arg + 1);
				
				free(aux);
			}

			cmd->argv[argc++] = arg;
		}
	}

	cmd->argv[argc] = (char*)NULL;

	return (struct cmd*)cmd;
}

static void runcmd(struct cmd* cmd) {

	struct execcmd exec;

	switch (cmd->type) {
	
		case EXEC:
			exec = *(struct execcmd*)cmd;
			free(cmd);
			execvp(exec.argv[0], exec.argv);
			fprintf(stderr, "cannot exec %s. error: %s\n", exec.argv[0], strerror(errno));
			_exit(EXIT_FAILURE);
			break;

		case BACK: {
			// Change the current SIGCHLD handler with
			// *handler*
			struct sigaction act = (const struct sigaction){};
			act.sa_handler = handler;
			sigaction(SIGCHLD, &act, NULL);

			// forks again and doesn´t wait for it
			// to finish right now.
			// When the process finishes it will send a SIGCHLD
			// signal and it will notify its parent through 
			// the handler defined above.
			if ((back = fork()) == 0) {
				cmd->type = EXEC;
				runcmd(cmd);
			}
			break;
		}
	}
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

		// exit command must be called in the father
		if (strcmp(cmd, "exit") == 0)
			_exit(EXIT_SUCCESS);

		// forks and run the command
		if ((p = fork()) == 0)
			runcmd(parsecmd(cmd));
		
		// waits for the process to finish
		waitpid(p, &status, 0);

		if (WIFEXITED(status)) {

			fprintf(stdout, "%s	Program: %s exited, status: %d %s\n", COLOR_BLUE, cmd, WEXITSTATUS(status), COLOR_RESET);
			status = WEXITSTATUS(status);

		} else if (WIFSIGNALED(status)) {

			fprintf(stdout, "%s	Program: %s killed, status: %d %s\n", COLOR_BLUE, cmd, -WTERMSIG(status), COLOR_RESET);
			status = -WTERMSIG(status);

		} else if (WTERMSIG(status)) {

			fprintf(stdout, "%s	Program: %s stopped, status: %d %s\n", COLOR_BLUE, cmd, -WSTOPSIG(status), COLOR_RESET);
			status = -WSTOPSIG(status);
		}
	}
	
	return 0;
}
