#include "runcmd.h"

// runs the command in 'cmd'
int run_cmd(char* cmd) {
	
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

