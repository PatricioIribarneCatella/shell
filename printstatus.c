#include "printstatus.h"

// prints information of process' status
void print_status_info(char* cmd) {
	
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

// prints info when a background process is spawned
void print_back_info(int back_pid) {

	fprintf(stdout, "%s  [PID=%d] %s\n",
		COLOR_BLUE, back_pid, COLOR_RESET);
}

