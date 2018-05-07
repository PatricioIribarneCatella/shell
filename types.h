#ifndef TYPES_H
#define TYPES_H

/* Commands definition types */

// command struct interface
struct cmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
};

// exec command struct
struct execcmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	int argc;
	int eargc;
	char* argv[MAXARGS];
	char* eargv[MAXARGS];
};

// file struct
struct file {
	int flags;
	char name[FNAMESIZE];
};

// redirection command struct
struct redircmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	struct file out;
	struct file in;
	struct file err;
	struct cmd* c;
};

// pipe command struct
struct pipecmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	struct cmd* leftcmd;
	struct cmd* rightcmd;
};

// background command struct
struct backcmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	struct cmd* c;
};

#endif // TYPES_H
