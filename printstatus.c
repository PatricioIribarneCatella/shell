#include "printstatus.h"

static void print_pipe_info(struct cmd* c) {

	struct pipecmd* p;

	if (c->type == PIPE) {
		
		p = (struct pipecmd*)c;

		print_pipe_info(p->leftcmd);

		printf("%s", "|");

		print_pipe_info(p->rightcmd);

		return;
	}
	
	fprintf(stdout, " %s ", c->scmd);

	return;
}

// prints information of process' status
void print_status_info(struct cmd* cmd) {

	if (strlen(cmd->scmd) == 0
		&& cmd->type != PIPE)
		return;
	
	if (cmd->type == PIPE) {

		fprintf(stdout, "%s	Command: [", COLOR_BLUE);
		
		print_pipe_info(cmd);
		
		fprintf(stdout, "] done %s\n", COLOR_RESET);
	
		// Sets global variable 'status'
		status = WEXITSTATUS(status);

		return;
	}

	if (WIFEXITED(status)) {

		fprintf(stdout, "%s	Program: [%s] exited, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, WEXITSTATUS(status), COLOR_RESET);
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {

		fprintf(stdout, "%s	Program: [%s] killed, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, -WTERMSIG(status), COLOR_RESET);
		status = -WTERMSIG(status);

	} else if (WTERMSIG(status)) {

		fprintf(stdout, "%s	Program: [%s] stopped, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, -WSTOPSIG(status), COLOR_RESET);
		status = -WSTOPSIG(status);
	}
}

// prints info when a background process is spawned
void print_back_info(struct cmd* back) {

	fprintf(stdout, "%s  [PID=%d] %s\n",
		COLOR_BLUE, back->pid, COLOR_RESET);
}

