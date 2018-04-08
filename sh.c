#include "defs.h"
#include "types.h"
#include "readline.h"
#include "freecmd.h"
#include "runcmd.h"

/* Global variables */
stack_t ss;
struct cmd* parsed_back;
struct cmd* parsed_pipe;
pid_t back = 0;
int status = 0;
int background = 0;
char promt[PRMTLEN] = {0};
char back_cmd[BUFLEN] = {0};

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

