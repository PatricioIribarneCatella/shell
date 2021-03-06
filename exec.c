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

// opens the file and redirects it
// to the appropiate standar flow
static void redir_std_flow(struct cmd* cmd,
		struct file f, int redir_fd) {

	int fd;
	char buf[BUFLEN];

	if (strlen(f.name) > 0) {
		if (strcmp(f.name, "&1") == 0)
			fd = STDOUT_FILENO;
		else if ((fd = open_redir_fd(f.name, f.flags)) < 0) {
			memset(buf, 0, BUFLEN);
			snprintf(buf, sizeof buf, "cannot open file: %s ", f.name);
			perror(buf);
			free(ss.ss_sp);
			free_command(cmd);
			_exit(EXIT_FAILURE);
		}
		if (dup2(fd, redir_fd) < 0) {
			memset(buf, 0, BUFLEN);
			snprintf(buf, sizeof buf, "cannot dup file: %s ", f.name);
			perror(buf);
			free(ss.ss_sp);
			free_command(cmd);
			_exit(EXIT_FAILURE);
		}
	}
}

// executes a command - does not return
void exec_cmd(struct cmd* cmd) {

	struct redircmd* r;
	struct execcmd* e;
	struct pipecmd* p;
	struct backcmd* b;
	char buf[BUFLEN];
	int pfd[2];
	int exit_code, s;
	pid_t auxp, rp;
	
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
			
			// stdin redirection
			redir_std_flow(cmd, r->in, STDIN_FILENO);
		
			// stdout redirection
			redir_std_flow(cmd, r->out, STDOUT_FILENO);
		
			// stderr redirection
			redir_std_flow(cmd, r->err, STDERR_FILENO);
			
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
			
			rp = fork();

			if (rp == 0) {
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

			// waits for both childs
			// and watches which one is the
			// right command. Checks the status
			// code to forward it to the
			// main 'shell' process.
			auxp = wait(&s);

			if (auxp == rp)
				exit_code = WEXITSTATUS(s);

			auxp = wait(&s);
			
			if (auxp == rp)
				exit_code = WEXITSTATUS(s);
			
			_exit(exit_code);
			break;
		}
	}
}

