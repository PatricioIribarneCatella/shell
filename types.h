#ifndef TYPES_H
#define TYPES_H

/* Commands definition types */

struct cmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
};

struct execcmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	int argc;
	int eargc;
	char* argv[MAXARGS];
	char* eargv[MAXARGS];
};

struct redircmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	int flags;
	char out_file[FNAMESIZE];
	char in_file[FNAMESIZE];
	char err_file[FNAMESIZE];
	struct cmd* c;
};

struct pipecmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	struct cmd* leftcmd;
	struct cmd* rightcmd;
};

struct backcmd {
	int type;
	pid_t pid;
	char scmd[BUFLEN];
	struct cmd* c;
};

#endif // TYPES_H
