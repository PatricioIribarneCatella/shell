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
		flags | O_CLOEXEC,
		S_IRUSR | S_IWUSR);

	return fd;
}

// executes a command - does not return
void exec_cmd(struct cmd* cmd) {

	struct redircmd* r;
	struct execcmd* e;
	struct pipecmd* p;
	struct backcmd* b;
	char buf[BUFLEN];
	int pfd[2];

	switch (cmd->type) {

		case EXEC:
			e = (struct execcmd*)cmd;

			set_environ_vars(e->eargv, e->eargc);
			
			execvp(e->argv[0], e->argv);	
		
			memset(buf, 0, BUFLEN);
			snprintf(buf, sizeof buf, "cannot exec %s ", e->argv[0]);
			perror(buf);
			
			break;

		case BACK: {
			b = (struct backcmd*)cmd;

			// sets the current 
			// process group id
			// to the PID of the 
			// calling process
			setpgid(0, 0);
			
			exec_cmd(b->c);
			break;
		}

		case REDIR: {
			// changes the input/output flow
			r = (struct redircmd*)cmd;
			int fd_in, fd_out, fd_err;
			
			// stdin redirection
			if (strlen(r->in.name) > 0) {
				if ((fd_in = open_redir_fd(r->in.name, r->in.flags)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r->in.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
				if (dup2(fd_in, STDIN_FILENO) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot dup stdin file: %s ", r->in.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
			}
			
			// stdout redirection
			if (strlen(r->out.name) > 0) {
				if ((fd_out = open_redir_fd(r->out.name, r->out.flags)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r->out.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
				if (dup2(fd_out, STDOUT_FILENO) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot dup stdout file: %s ", r->out.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
			}

			// stderr redirection
			if (strlen(r->err.name) > 0) {
				if (strcmp(r->err.name, "&1") == 0) {
					fd_err = STDOUT_FILENO;
				}
				else if ((fd_err = open_redir_fd(r->err.name, r->err.flags)) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot open file: %s ", r->err.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
				if (dup2(fd_err, STDERR_FILENO) < 0) {
					memset(buf, 0, BUFLEN);
					snprintf(buf, sizeof buf, "cannot dup stderr file: %s ", r->err.name);
					perror(buf);
					free(ss.ss_sp);
					free_command(cmd);
					_exit(EXIT_FAILURE);
				}
			}
			
			exec_cmd(r->c);

			free(ss.ss_sp);
			free_command(cmd);

			_exit(EXIT_FAILURE);

			break;
		}
		
		case PIPE: {
			// pipes two commands
			p = (struct pipecmd*)cmd;
			
			if (pipe(pfd) < 0) {
				memset(buf, 0, sizeof buf);
				snprintf(buf, sizeof buf, "pipe creation failed ");
				perror(buf);
				_exit(EXIT_FAILURE);
			}
			
			if (fork() == 0) {
				dup2(pfd[WRITE], STDOUT_FILENO);
				close(pfd[READ]);
				close(pfd[WRITE]);
				exec_cmd(p->leftcmd);
			}
			
			if (fork() == 0) {
				dup2(pfd[READ], STDIN_FILENO);
				close(pfd[READ]);
				close(pfd[WRITE]);
				exec_cmd(p->rightcmd);
			}
			
			close(pfd[READ]);
			close(pfd[WRITE]);
		
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

