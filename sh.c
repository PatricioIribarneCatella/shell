#include "defs.h"
#include "types.h"
#include "readline.h"
#include "createcmd.h"
#include "freecmd.h"
#include "printstatus.h"
#include "parsing.h"
#include "utils.h"
#include "builtin.h"
#include "exec.h"

/* Global variables */
static pid_t back;
static char back_cmd[BUFLEN];

stack_t ss;
struct cmd* parsed_back;
struct cmd* parsed_pipe;
int status = 0;
int background = 0;
char promt[PRMTLEN] = {0};

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

// runs the command in 'cmd'
static int run_cmd(char* cmd) {
	
	pid_t p, w;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the promt again
	if (cmd[0] == END_STRING)
		return 0;

	// chdir has to be called within the father,
	// not the child.
	if (cd(cmd))
		return 0;

	// exit command must be called in the father
	if (exit_shell(cmd))
		return EXIT_SHELL;

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

		return 0;
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

	return 0;
}

// runs a shell command
static void run_shell() {

	char* cmd;

	while ((cmd = read_line(promt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// frees the space 
// of the handler´s stack
static void end_shell() {
	
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
	ss.ss_flags = 0;

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

	end_shell();

	return 0;
}

