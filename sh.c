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

// Fd for pipes
#define READ 0
#define WRITE 1

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
static char back_cmd[BUFLEN];
static char buffer[BUFLEN];
static char promt[PRMTLEN];
//static char right[BUFLEN];
//static char left[BUFLEN];
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

// prints information of process´ status
static void print_status_info(char* cmd) {
	
	if (strlen(cmd) == 0)
		return;

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

// prints information of background process´ status
static void print_background_info() {
	
	if ((back != 0) && waitpid(back, &status, WNOHANG) > 0)
		fprintf(stdout, "%s	process %d done [%s], status: %d %s\n",
			COLOR_BLUE, back, back_cmd, WEXITSTATUS(status), COLOR_RESET);
}

// exists nicely
static void exit_shell(char* cmd) {

	if (strcmp(cmd, "exit") == 0)
		_exit(EXIT_SUCCESS);
}

// returns true if "chdir" was performed
static int cd(char* cmd) {

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

		if (chdir(dir) < 0)
			fprintf(stderr, "cannot cd to %s. error: %s\n",
				dir, strerror(errno));
		else {
			memset(promt, 0, PRMTLEN);
			strcat(promt, "(");
			char* cwd = get_current_dir_name();
			strcat(promt, cwd);
			free(cwd);
			strcat(promt, ")");
			status = 0;
		}

		return 1;
	}

	return 0;
}

static char* read_line(const char* promt) {

	fprintf(stdout, "%s %s %s\n", COLOR_RED, promt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");

	memset(buffer, 0, BUFLEN);

	int i = 0;
	int c = getchar();

	while (c != END_LINE && c != EOF) {

		buffer[i++] = c;

		c = getchar();
	}

	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

// sets the "key" argument with the key part of
// the "arg" argument and null-terminates it
static void get_environ_key(char* arg, char* key) {

	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets the "value" argument with the value part of
// the "arg" argument and null-terminates it
static void get_environ_value(char* arg, char* value, int idx) {

	int i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

static void exec_cmd(struct cmd* cmd) {

	struct execcmd exec;
	struct execcmd redir;
	struct pipecmd pipe_cmd;
	int p[2];

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
			exec_cmd(cmd);
			break;
		}

		case REDIR: {
			// changes the input/output flow
			redir = *(struct execcmd*)cmd;
			if (redir.fd_in >= 0)
				dup2(redir.fd_in, STDIN_FILENO);
			if (redir.fd_out >= 0)
				dup2(redir.fd_out, STDOUT_FILENO);
			cmd->type = EXEC;
			exec_cmd(cmd);
			break;
		}
		
		case PIPE: {
			// pipes two commands
			pipe_cmd = *(struct pipecmd*)cmd;
			if (pipe(p) < 0) {
				fprintf(stderr, "pipe creation failed. error: %s\n",
					strerror(errno));
				_exit(EXIT_FAILURE);
			}
			if (fork() == 0) {
				dup2(p[WRITE], STDOUT_FILENO);
				close(p[READ]);
				close(p[WRITE]);
				exec_cmd(pipe_cmd.leftcmd);
			}
			if (fork() == 0) {
				dup2(p[READ], STDIN_FILENO);
				close(p[READ]);
				close(p[WRITE]);
				exec_cmd(pipe_cmd.rightcmd);
			}
			close(p[READ]);
			close(p[WRITE]);
			wait(NULL);
			wait(NULL);
			break;
		}
	}
}

static char* get_arg(char* buf, int idx) {

	char* arg = malloc(ARGSIZE);
	memset(arg, 0, ARGSIZE);
	int i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		arg[i] = buf[idx];
		i++; idx++;
	}

	return arg;
}

static int open_redir_fd(char* file) {

	int fd;

	fd = open(file,
			O_APPEND | O_CLOEXEC | O_RDWR | O_CREAT,
			S_IRUSR | S_IWUSR);

	return fd;
}

// looks in a block for the 'c' character
// and returns the index in which it is, or -1
// in other case
static int block_contains(char* buf, char c) {

	for (int i = 0; i < strlen(buf); i++)
		if (buf[i] == c)
			return i;
	
	return -1;
}

static bool redir_flow(struct execcmd* c, char* arg) {

	int fd, inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
	
		if ((fd = open_redir_fd(arg + outIdx + 1)) < 0)
			fprintf(stderr, "Cannot open redir file at: %s. error: %s",
					arg + outIdx + 1, strerror(errno));
		else {
			c->fd_out = fd;
			c->type = REDIR;
			return true;
		}
	}

	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		
		if ((fd = open_redir_fd(arg + inIdx + 1)) < 0)
			fprintf(stderr, "Cannot open redir file at: %s. error: %s",
					arg + inIdx + 1, strerror(errno));
		else {
			c->fd_in = fd;
			c->type = REDIR;
			return true;
		}
	}
	
	return false;
}

static bool set_environ_var(char* arg) {

	int equalIndex;

	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if ((equalIndex = block_contains(arg, '=')) > 0) {

		char key[50], value[50];

		get_environ_key(arg, key);
		get_environ_value(arg, value, equalIndex);
		
		if (block_contains(key, '-') < 0)
			setenv(key, value, 1);
		
		return true;
	}
	
	return false;
}

static char* expand_environ_var(char* arg) {

	// expand environment variables
	if (arg[0] == '$') {

		char* aux = arg;

		if (arg[1] == '?')
			arg = itoa(status);
		else
			arg = getenv(arg + 1);

		free(aux);
	}
	
	return arg;
}

static struct cmd* exec_cmd_create(int back) {

	struct execcmd* e = malloc(sizeof(*e));
	memset(e, 0, sizeof(*e));

	e->fd_in = -1;
	e->fd_out = -1;
	e->type = EXEC;
	
	if (back)
		e->type = BACK;
	
	return (struct cmd*)e;
}

static struct cmd* parse_exec(char* buf_cmd, int back) {

	struct execcmd* c;
	char* arg;
	int idx = 0, argc = 0;
	
	c = (struct execcmd*)exec_cmd_create(back);
	
	while (buf_cmd[idx] != END_STRING) {
	
		arg = get_arg(buf_cmd, idx);
		idx = idx + strlen(arg);
		
		if (buf_cmd[idx] != END_STRING)
			idx++;
		
		arg = expand_environ_var(arg);
		
		if (redir_flow(c, arg))
			continue;
		
		if (set_environ_var(arg))
			continue;
		
		c->argv[argc++] = arg;
	}
	
	c->argv[argc] = (char*)NULL;
	
	return (struct cmd*)c;
}

static struct cmd* parse_back(char* buf_cmd) {

	int i = 0;

	while (buf_cmd[i] != '&')
		i++;
	
	buf_cmd[i] = END_STRING;
	
	return parse_exec(buf_cmd, 1);
}

static struct cmd* parse_cmd(char* buf_cmd) {

	if (strlen(buf_cmd) == 0)
		return NULL;

	if (block_contains(buf_cmd, '&') >= 0)
		return parse_back(buf_cmd);
		
	return parse_exec(buf_cmd, 0);
}

static struct cmd* pipe_cmd_create(struct cmd* left, struct cmd* right) {

	if (!right)
		return left;
	
	struct pipecmd* p = malloc(sizeof(*p));
	memset(p, 0, sizeof(*p));
	
	p->type = PIPE;
	p->leftcmd = left;
	p->rightcmd = right;
	
	return (struct cmd*)p;
}

static char* split_line(char* buf, char splitter) {

	int i = 0;

	while (buf[i] != splitter &&
			buf[i] != END_STRING)
		i++;
		
	buf[i] = END_STRING;
	
	return &buf[i++];
}

static struct cmd* parse_line(char* buf) {
	
	char* right = split_line(buf, '|');
	
	struct cmd* l = parse_cmd(buf);
	struct cmd* r = parse_cmd(right);
	
	return pipe_cmd_create(l, r);
}

// runs the command in 'cmd'
static void run_cmd(char* cmd) {
	
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the promt again
	if (cmd[0] == END_STRING)
		return;
	
	// chdir has to be called within the father,
	// not the child.
	if (cd(cmd))
		return;

	// exit command must be called in the father
	exit_shell(cmd);

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0)
		exec_cmd(parsed);

	// doesn´t wait for it to finish
	if (parsed->type == BACK) {
		strcpy(back_cmd, cmd);
		back = p;
		return;
	}

	// waits for the process to finish
	waitpid(p, &status, 0);
}

static void run_shell() {

	char* cmd;

	while ((cmd = read_line(promt)) != NULL) {

		// if the process in background finished,
		// print info about it
		print_background_info();
		
		run_cmd(cmd);

		print_status_info(cmd);
	}
}

// initialize the shell
// with the "HOME" directory
static void init_shell() {

	char* home = getenv("HOME");

	if (chdir(home) < 0)
		fprintf(stderr, "cannot cd to %s. error: %s\n",
			home, strerror(errno));
	else {
		strcat(promt, "(");
		strcat(promt, home);
		strcat(promt, ")");
	}
}

int main(int argc, char const *argv[]) {

	init_shell();

	run_shell();
	
	return 0;
}

