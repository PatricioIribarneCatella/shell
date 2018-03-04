#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

s
#define COLOR_BLUE "\x1b[34m"
#define COLOR_RED "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

#define END_STRING '\0'
#define END_LINE '\n'
#define SPACE ' '

#define BUFLEN 1024
#define PRMTLEN 1024
#define MAXARGS 20
#define ARGSIZE 100

// Command representation after parsed
#define EXEC 1
#define BACK 2
#define REDIR 3
#define PIPE 4

struct cmd {
	int type;
};

struct execcmd {
	int type;
	int fd_in;
	int fd_out;
	char* argv[MAXARGS];
	char* eargv[MAXARGS];
};

struct pipecmd {
	int type;
	struct cmd* leftcmd;
	struct cmd* rightcmd;
};

int status;
pid_t back;
static char buffer[BUFLEN];
static char promt[PRMTLEN];
static char numbers[10] = "0123456789";
static char bufNum[32];

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
	int c = getchar();

	while (c != END_LINE && c != EOF) {

		buffer[i++] = c;

		c = getchar();
	}

	if (c == END_LINE)
		buffer[i++] = END_LINE;

	buffer[i] = END_STRING;

	return (buffer[0] != 0) ? buffer : NULL;
}

// looks in the argument for the 'c' character
// and returns the index in which it is, or -1
// in other case
static int argContains(char* arg, char c) {

	for (int i = 0; i < strlen(arg); i++)
		if (arg[i] == c)
			return i;
	return -1;
}

// sets the "key" argument with the key part of
// the "arg" argument and null-terminates it
static void getEnvironKey(char* arg, char* key) {

	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets the "value" argument with the value part of
// the "arg" argument and null-terminates it
static void getEnvironValue(char* arg, char* value, int idx) {

	int i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

static struct cmd* parsecmd(char* buf) {

	struct execcmd* cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));

	cmd->fd_in = -1;
	cmd->fd_out = -1;
	cmd->type = EXEC;

	int fd, idx,
		equalIndex, redirOutputIndex, redirInputIndex,
		argc = 0, i = 0;

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

			// flow redirection for output
			if ((redirOutputIndex = argContains(arg, '>')) >= 0) {

				if ((fd = open(arg + redirOutputIndex + 1,
						O_APPEND | O_CLOEXEC | O_RDWR | O_CREAT,
						S_IRUSR | S_IWUSR)) < 0)
					fprintf(stderr, "Cannot open redir file at: %s. error: %s",
							arg + redirOutputIndex + 1, strerror(errno));
				else {
					cmd->fd_out = fd;
					cmd->type = REDIR;
				}

				continue;
			}

			// flow redirection for input
			if ((redirInputIndex = argContains(arg, '<')) >= 0) {

				if ((fd = open(arg + redirInputIndex + 1,
						O_APPEND | O_CLOEXEC | O_RDWR | O_CREAT,
						S_IRUSR | S_IWUSR)) < 0)
					fprintf(stderr, "Cannot open redir file at: %s. error: %s",
							arg + redirInputIndex + 1, strerror(errno));
				else {
					cmd->fd_in = fd;
					cmd->type = REDIR;
				}

				continue;

			}

			// sets environment variables apart from the
			// ones defined in the global variable "environ"
			if ((equalIndex = argContains(arg, '=')) > 0) {

				char key[50], value[50];

				getEnvironKey(arg, key);
				getEnvironValue(arg, value, equalIndex);

				setenv(key, value, 1);

				continue;
			}

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
	struct execcmd redir;

	switch (cmd->type) {

		case EXEC:
			exec = *(struct execcmd*)cmd;
			free(cmd);
			execvp(exec.argv[0], exec.argv);
			fprintf(stderr, "cannot exec %s. error: %s\n",
				exec.argv[0], strerror(errno));
			_exit(EXIT_FAILURE);
			break;

		case BACK: {
			// sets the current process group id to 0
			setpgid(0, 0);
			cmd->type = EXEC;
			runcmd(cmd);
			break;
		}

		case REDIR: {
			// changes the input/output flow
			redir = *(struct execcmd*)cmd;
			if (redir.fd_in >= 0)
				dup2(redir.fd_in, 0);
			if (redir.fd_out >= 0)
				dup2(redir.fd_out, 1);
			cmd->type = EXEC;
			runcmd(cmd);
			break;
		}
	}
}

int main(int argc, char const *argv[]) {

	pid_t p;
	char* cmd;

	char* home = getenv("HOME");

	if (chdir(home) < 0)
		fprintf(stderr, "cannot cd to %s. error: %s\n",
			home, strerror(errno));
	else {
		strcat(promt, "(");
		strcat(promt, home);
		strcat(promt, ")");
	}

	while ((cmd = readline(promt)) != NULL) {

		if ((back != 0) && waitpid(back, &status, WNOHANG) > 0)
			printf("	process %d done\n", back);

		// if the "enter" key is pressed just
		// print the promt again
		if (cmd[0] == END_LINE)
			continue;

		cmd[strlen(cmd) - 1] = END_STRING; // chop \n

		// chdir has to be called within the father, not the child.
		if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {

			if (chdir(cmd + 3) < 0)
				fprintf(stderr, "cannot cd to %s. error: %s\n",
					cmd + 3, strerror(errno));
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

		// parses the command line
		struct cmd *parsedCmd = parsecmd(cmd);

		if (parsedCmd->type == BACK) {
			// forks and run the command
			if ((back = fork()) == 0) {
				runcmd(parsedCmd);
			}
			// it doesn´t wait for it to finish
			continue;
		}

		// forks and run the command
		if ((p = fork()) == 0) {
			runcmd(parsedCmd);
		}

		// waits for the process to finish
		waitpid(p, &status, 0);

		if (WIFEXITED(status)) {

			fprintf(stdout, "%s	Program: %s exited, status: %d %s\n",
				COLOR_BLUE, cmd, WEXITSTATUS(status), COLOR_RESET);
			status = WEXITSTATUS(status);

		} else if (WIFSIGNALED(status)) {

			fprintf(stdout, "%s	Program: %s killed, status: %d %s\n",
				COLOR_BLUE, cmd, -WTERMSIG(status), COLOR_RESET);
			status = -WTERMSIG(status);

		} else if (WTERMSIG(status)) {

			fprintf(stdout, "%s	Program: %s stopped, status: %d %s\n",
				COLOR_BLUE, cmd, -WSTOPSIG(status), COLOR_RESET);
			status = -WSTOPSIG(status);
		}
	}

	return 0;
}
