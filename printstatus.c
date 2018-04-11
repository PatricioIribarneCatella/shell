#include "printstatus.h"

static void print_pipe_info(struct cmd* c) {

	struct pipecmd* p;

	p = (struct pipecmd*)c;

	if (c->type == PIPE) {
		
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

	if (strlen(cmd->scmd) == 0)
		return;
	
	if (cmd->type == PIPE) {
		printf("%s", "Command: [");
		print_pipe_info(cmd);
		printf("%s\n", "] done");
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

