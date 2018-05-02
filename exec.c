#include "exec.h"

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

// opens the file in which the stdin or
// stdout flow will be redirected
static int open_redir_fd(char* file, int flags) {

	int fd;

	fd = open(file,
		flags | O_CLOEXEC | O_CREAT,
		S_IRUSR | S_IWUSR);

	return fd;
}

// executes a command - does not return
void exec_cmd(struct cmd* cmd) {

	struct execcmd e_cmd;
	struct execcmd r_cmd;
	struct pipecmd p_cmd;
	struct backcmd b_cmd;
	char buf[BUFLEN];
	int p[2];

	switch (cmd->type) {

		case EXEC:
			e_cmd = *(struct execcmd*)cmd;

			set_environ_vars(e_cmd.eargv, e_cmd.eargc);
			
			execvp(e_cmd.argv[0], e_cmd.argv);	
		
			memset(buf, 0, BUFLEN);
			snprintf(buf, sizeof buf, "cannot exec %s ", e_cmd.argv[0]);
			perror(buf);
			
			free(ss.ss_sp);
			free_command(cmd);
			
			_exit(EXIT_FAILURE);
			break;

		case BACK: {
			b_cmd = *(struct backcmd*)cmd;

			// sets the current 
			// process group id
			// to the PID of the 
			// calling process
			setpgid(0, 0);
			
			exec_cmd(b_cmd.c);
			break;
		}

		case REDIR: {
			// changes the input/output flow
			r_cmd = *(struct execcmd*)cmd;
			int fd_in, fd_out, fd_err;
			
			// stdin redirection
			if (strlen(r_cmd.in_file) > 0) {
				if ((fd_in = open_redir_fd(r_cmd.in_file, O_RDONLY)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r_cmd.in_file);
					perror(buf);
					_exit(EXIT_FAILURE);
				}
				if (fd_in >= 0)
					dup2(fd_in, STDIN_FILENO);
			}
			
			// stdout redirection
			if (strlen(r_cmd.out_file) > 0) {
				if ((fd_out = open_redir_fd(r_cmd.out_file, O_WRONLY | O_TRUNC)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r_cmd.out_file);
					perror(buf);
					_exit(EXIT_FAILURE);
				}
				if (fd_out >= 0)
					dup2(fd_out, STDOUT_FILENO);
			}

			// stderr redirection
			if (strlen(r_cmd.err_file) > 0) {
				if (strcmp(r_cmd.err_file, "&1") == 0) {
					fd_err = STDOUT_FILENO;
				}
				else if ((fd_err = open_redir_fd(r_cmd.err_file, O_WRONLY | O_TRUNC)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r_cmd.err_file);
					perror(buf);
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
			p_cmd = *(struct pipecmd*)cmd;
			
			if (pipe(p) < 0) {
				memset(buf, 0, sizeof buf);
				snprintf(buf, sizeof buf, "pipe creation failed ");
				perror(buf);
				_exit(EXIT_FAILURE);
			}
			
			if (fork() == 0) {
				dup2(p[WRITE], STDOUT_FILENO);
				close(p[READ]);
				close(p[WRITE]);
				exec_cmd(p_cmd.leftcmd);
			}
			
			if (fork() == 0) {
				dup2(p[READ], STDIN_FILENO);
				close(p[READ]);
				close(p[WRITE]);
				exec_cmd(p_cmd.rightcmd);
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

