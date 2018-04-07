#include "defs.h"
#include "types.h"
#include "readline.h"
#include "createcmd.h"
#include "freecmd.h"
#include "printstatus.h"
#include "parsing.h"
#include "utils.h"

/* Global variables */
static pid_t back;
static stack_t ss;
static struct cmd* parsed_back;
static struct cmd* parsed_pipe;

static char back_cmd[BUFLEN];
static char promt[PRMTLEN];

int status = 0;
int background = 0;

/* Handler function for SIGCHLD signal */
void sig_handler(int num) {
	
	fflush(stdout);

	if (back != 0 && waitpid(back, &status, WNOHANG) > 0) {
		fprintf(stdout, "%s	process %d done [%s], status: %d %s\n",
			COLOR_BLUE, back, back_cmd, WEXITSTATUS(status), COLOR_RESET);
		background = 1;
		free_back_command(parsed_back);
	}
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
	struct backcmd back_cmd;
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
			back_cmd = *(struct backcmd*)cmd;

			// sets the current 
			// process group id
			// to the PID of the 
			// calling process
			setpgid(0, 0);
			
			exec_cmd(back_cmd.c);
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

