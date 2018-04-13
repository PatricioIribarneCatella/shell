#include "defs.h"
#include "types.h"
#include "readline.h"
#include "freecmd.h"
#include "runcmd.h"

/* Global variables */
stack_t ss;
char promt[PRMTLEN] = {0};
int background = 0;

/* Extern variables */
extern int status;
extern struct cmd* back_cmds;

/* Handler function for SIGCHLD signal */
void sig_handler(int num) {
	
	char buf[BUFLEN] = {0};
	fflush(stdout);

	if (back_cmds != NULL && waitpid(back_cmds->pid, &status, WNOHANG) > 0) {
	
		snprintf(buf, sizeof buf, "%s	process %d done [%s], status: %d %s\n",
			COLOR_BLUE, back_cmds->pid, back_cmds->scmd, WEXITSTATUS(status), COLOR_RESET);
		
		write(STDOUT_FILENO, buf, sizeof buf);
		
		free_command(back_cmds);

		background = 1;
	}
}

// runs a shell command
static void run_shell() {

	char* cmd;

	while ((cmd = read_line(promt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initialize the shell
// with the "HOME" directory
static void init_shell() {

	char buf[BUFLEN] = {0};
	char* home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(promt, sizeof promt, "(%s)", home);
		setenv("PWD", home, 1);
		setenv("OLDPWD", home, 1);
	}
	
	// allocates space 
	// for the new handler´s stack
	ss.ss_sp = malloc(SIGSTKSZ);
	ss.ss_size = SIGSTKSZ;
	ss.ss_flags = 0;

	sigaltstack(&ss, NULL);
	
	// set signal handler for
	// SIGCHLD signal catching
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_ONSTACK;

	// blocks any other signal
	// interrupt when the handler
	// is being executed
	sigfillset(&act.sa_mask);
	
	sigaction(SIGCHLD, &act, NULL);
}

// frees the space 
// of the handler´s stack
static void end_shell() {
	
	free(ss.ss_sp);
}

int main(void) {

	init_shell();

	run_shell();

	end_shell();

	return 0;
}

