#ifndef TYPES_H
#define TYPES_H

/* Commands definition types */

struct cmd {
	int type;
};

struct execcmd {
	int type;
	int argc;
	char out_file[FNAMESIZE];
	char in_file[FNAMESIZE];
	char err_file[FNAMESIZE];
	char* argv[MAXARGS];
	char* eargv[MAXARGS];
};

struct pipecmd {
	int type;
	struct cmd* leftcmd;
	struct cmd* rightcmd;
};

#endif // TYPES_H
