#include "defs.h"
#include "types.h"

/* Global variables */
static int status;
static pid_t back;
static stack_t ss;
static int background;
static struct cmd* parsed_back;
static struct cmd* parsed_pipe;

static char back_cmd[BUFLEN];
static char buffer[BUFLEN];
static char promt[PRMTLEN];

// frees the memory allocated 
// for the tree structure command
static void free_command(struct cmd* cmd) {

	int i;
	struct pipecmd* p;
	struct execcmd* e;

	if (cmd->type == PIPE) {
		
		p = (struct pipecmd*)cmd;
		
		free_command(p->leftcmd);
		free_command(p->rightcmd);
		
		free(p);
		return;
	}

	e = (struct execcmd*)cmd;

	for (i = 0; i < e->argc; i++)
		free(e->argv[i]);

	for (i = 0; i < e->eargc; i++)
		free(e->eargv[i]);

	free(e);
}

/* Handler function for SIGCHLD signal */
void sig_handler(int num) {
	
	fflush(stdout);

	if (back != 0 && waitpid(back, &status, WNOHANG) > 0) {
		fprintf(stdout, "%s	process %d done [%s], status: %d %s\n",
			COLOR_BLUE, back, back_cmd, WEXITSTATUS(status), COLOR_RESET);
		background = 1;
		free_command(parsed_back);
	}
}

// prints information of process' status
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

static void print_back_info(int back_pid) {
	fprintf(stdout, "%s  [PID=%d] %s\n",
		COLOR_BLUE, back_pid, COLOR_RESET);
}

// exists nicely
static void exit_shell(char* cmd) {

	if (strcmp(cmd, "exit") == 0) {

		// frees the space 
		// of the handler´s stack
		free(ss.ss_sp);
		
		_exit(EXIT_SUCCESS);
	}
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

// read a line from the standar input
// and prints the prompt
static char* read_line(const char* promt) {

	int i = 0,
	    c = 0;

	memset(buffer, 0, BUFLEN);
	
	// signal handler sets
	// 'background' in true
	if (background) {
		background = 0;
		return buffer;
	}

	fprintf(stdout, "%s %s %s\n", COLOR_RED, promt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}
		
	// if the user press ctrl+D
	// just exit normally
	if (c == EOF && !background)
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

// opens the file in which the stdin or
// stdout flow will be redirected
static int open_redir_fd(char* file) {

	int fd;

	fd = open(file,
		O_TRUNC | O_CLOEXEC | O_RDWR | O_CREAT,
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

// sets the environment variables passed
// in the command line
static void set_environ_vars(char** eargv, int eargc) {

	int equalIndex;
	char key[50], value[50];

	for (int i = 0; i < eargc; i++) {

		equalIndex = block_contains(eargv[i], '=');

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, equalIndex);

		setenv(key, value, 1);
	}
} 

// executes a command - does not return
static void exec_cmd(struct cmd* cmd) {

	struct execcmd exec;
	struct execcmd redir;
	struct pipecmd pipe_cmd;
	int p[2];

	switch (cmd->type) {

		case EXEC:
			exec = *(struct execcmd*)cmd;

			set_environ_vars(exec.eargv, exec.eargc);
			
			execvp(exec.argv[0], exec.argv);
			
			fprintf(stderr, "cannot exec %s\n", exec.argv[0]);
			perror(NULL);

			_exit(EXIT_FAILURE);
			break;

		case BACK: {
			// sets the current 
			// process group id
			// to the PID of the 
			// calling process
			setpgid(0, 0);
			cmd->type = EXEC;
			
			exec_cmd(cmd);
			break;
		}

		case REDIR: {
			// changes the input/output flow
			redir = *(struct execcmd*)cmd;
			int fd_in, fd_out, fd_err;
			
			// stdin redirection
			if (strlen(redir.in_file) > 0) {
				if ((fd_in = open_redir_fd(redir.in_file)) < 0) {
					fprintf(stderr, "cannot open file: %s\n", redir.in_file);
					perror(NULL);
					_exit(EXIT_FAILURE);
				}
				if (fd_in >= 0)
					dup2(fd_in, STDIN_FILENO);
			}
			
			// stdout redirection
			if (strlen(redir.out_file) > 0) {
				if ((fd_out = open_redir_fd(redir.out_file)) < 0) {
					fprintf(stderr, "cannot open file: %s\n", redir.out_file);
					perror(NULL);
					_exit(EXIT_FAILURE);
				}
				if (fd_out >= 0)
					dup2(fd_out, STDOUT_FILENO);
			}

			// stderr redirection
			if (strlen(redir.err_file) > 0) {
				if (strcmp(redir.err_file, "&1") == 0) {
					fd_err = STDOUT_FILENO;
				}
				else if ((fd_err = open_redir_fd(redir.err_file)) < 0) {
					fprintf(stderr, "cannot open file: %s\n", redir.err_file);
					perror(NULL);
					_exit(EXIT_FAILURE);
				}
				if (fd_err >= 0)
					dup2(fd_err, STDERR_FILENO);
			}
			
			cmd->type = EXEC;
			exec_cmd(cmd);
			break;
		}
		
		case PIPE: {
			// pipes two commands
			pipe_cmd = *(struct pipecmd*)cmd;
			
			if (pipe(p) < 0) {
				fprintf(stderr, "pipe creation failed\n");
				perror(NULL);
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
		
			// free the memory allocated
			// for the pipe tree structure
			free_command(parsed_pipe);
			free(ss.ss_sp);

			wait(NULL);
			wait(NULL);
			
			_exit(EXIT_SUCCESS);
			break;
		}
	}
}

// parses an argument of the command stream input
static char* get_arg(char* buf, int idx) {

	char* arg;
	int i;
	
	arg = (char*)calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		arg[i] = buf[idx];
		i++; idx++;
	}

	return arg;
}


// parses and changes stdin/out/err if needed
static bool redir_flow(struct execcmd* c, char* arg) {

	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
			// stdout redir
			case 0: {
				strcpy(c->out_file, arg + 1);
				break;
			}
			// stderr redir
			case 1: {
				strcpy(c->err_file, &arg[outIdx + 1]);
				break;
			}
		}
		
		free(arg);
		c->type = REDIR;
		
		return true;
	}
	
	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file, arg + 1);
		
		c->type = REDIR;
		free(arg);
		
		return true;
	}
	
	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool parse_environ_var(struct execcmd* c, char* arg) {

	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if (block_contains(arg, '=') > 0) {

		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] = arg;
			return true;
		}
	}
	
	return false;
}

// expand environment variables
static char* expand_environ_var(char* arg) {

	if (arg[0] == '$') {

		char* aux;
		char s[10];

		if (arg[1] == '?') {
			sprintf(s, "%d", status);
			aux = s;
		}
		else
			aux = getenv(arg + 1);

		strcpy(arg, aux);
	}
	
	return arg;
}

// creates an execcmd struct to store 
// the args and environ vars of the command
static struct cmd* exec_cmd_create(int back) {

	struct execcmd* e;
	
	e = (struct execcmd*)calloc(sizeof(*e), sizeof(*e));
	e->type = EXEC;
	
	if (back)
		e->type = BACK;
	
	return (struct cmd*)e;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
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
		
		if (parse_environ_var(c, arg))
			continue;
		
		c->argv[argc++] = arg;
	}
	
	c->argv[argc] = (char*)NULL;
	c->argc = argc;
	
	return (struct cmd*)c;
}

// parses a command knowing that it contains
// the '&' char
static struct cmd* parse_back(char* buf_cmd) {

	int i = 0;

	while (buf_cmd[i] != '&')
		i++;
	
	buf_cmd[i] = END_STRING;
	
	return parse_exec(buf_cmd, 1);
}

// parses a command and checks if it contains
// the '&' (background process) character
static struct cmd* parse_cmd(char* buf_cmd) {

	if (strlen(buf_cmd) == 0)
		return NULL;
		
	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 &&
			buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);
		
	return parse_exec(buf_cmd, 0);
}

// encapsulates two commands into one pipe struct
static struct cmd* pipe_cmd_create(struct cmd* left, struct cmd* right) {

	if (!right)
		return left;
	
	struct pipecmd* p;

	p = (struct pipecmd*)calloc(sizeof(*p), sizeof(*p));
	
	p->type = PIPE;
	p->leftcmd = left;
	p->rightcmd = right;
	
	return (struct cmd*)p;
}

// splits a string line in two
// acording to the splitter character
static char* split_line(char* buf, char splitter) {

	int i = 0;

	while (buf[i] != splitter &&
		buf[i] != END_STRING)
		i++;
		
	buf[i++] = END_STRING;
	
	while (buf[i] == SPACE)
		i++;
	
	return &buf[i];
}

// parses the command line recursively
// looking for the pipe character '|'
static struct cmd* parse_line(char* buf) {
	
	struct cmd *r = NULL, *l;
	
	char* right = split_line(buf, '|');
	
	if (strlen(right) != 0)
		r = parse_line(right);

	l = parse_cmd(buf);
	
	return pipe_cmd_create(l, r);
}


// runs the command in 'cmd'
static void run_cmd(char* cmd) {
	
	pid_t p, w;
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
	if ((p = fork()) == 0) {
		
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		exec_cmd(parsed);
	}

	// doesn´t wait for it to finish
	if (parsed->type == BACK) {
		
		strcpy(back_cmd, cmd);
		
		// keep a reference
		// to the parsed back cmd
		// to free it when it finishes
		parsed_back = parsed;
		
		back = p;
		print_back_info(back);

		return;
	}

	// waits for the process to finish
	w = waitpid(p, &status, 0);
	
	// it checks if the return value
	// is negative and if errno is EINTR.
	// This could have happend
	// because of an interrupt 
	// by a SIGCHLD signal of
	// a process in background that finished.
	// It goes to the next instruction but the
	// waitpid call hasn´t finished yet,
	// so it try it again
	if (w < 0 && errno == EINTR)
		waitpid(p, &status, 0);

	if (parsed->type != PIPE)
		print_status_info(cmd);

	free_command(parsed);
}

static void run_shell() {

	char* cmd;

	while ((cmd = read_line(promt)) != NULL)
		run_cmd(cmd);

	// frees the space 
	// of the handler´s stack
	free(ss.ss_sp);
}

// initialize the shell
// with the "HOME" directory
static void init_shell() {

	char* home = getenv("HOME");

	if (chdir(home) < 0) {
		fprintf(stderr, "cannot cd to %s\n", home);
		perror(NULL);
	} else {
		strcat(promt, "(");
		strcat(promt, home);
		strcat(promt, ")");
	}
	
	// allocates space 
	// for the new handler´s stack
	ss.ss_sp = malloc(SIGSTKSZ);
	ss.ss_size = SIGSTKSZ;
	
	sigaltstack(&ss, NULL);
	
	// set signal handler for
	// SIGCHLD singal catching
	struct sigaction act = {0};
	act.sa_handler = sig_handler;
	act.sa_flags = SA_ONSTACK;

	// blocks any other signal
	// interrupt (SIGCHLD in this case)
	// when the handler
	// is being executed
	sigset_t sig;
	sigemptyset(&sig);
	sigaddset(&sig, SIGCHLD);
	act.sa_mask = sig;
	
	sigaction(SIGCHLD, &act, NULL);
}

int main(void) {

	init_shell();

	run_shell();
	
	return 0;
}

