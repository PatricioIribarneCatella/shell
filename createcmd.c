#include "createcmd.h"

// creates an execcmd struct to store 
// the args and environ vars of the command
struct cmd* exec_cmd_create() {

	struct execcmd* e;
	
	e = (struct execcmd*)calloc(sizeof(*e), sizeof(*e));
	e->type = EXEC;
	
	return (struct cmd*)e;
}

// creates a backcmd struct to store the
// background command to be executed
struct cmd* back_cmd_create(struct cmd* c) {

	struct backcmd* b;

	b = (struct backcmd*)calloc(sizeof(*b), sizeof(*b));
	b->type = BACK;
	b->c = c;

	return (struct cmd*)b;
}

// encapsulates two commands into one pipe struct
struct cmd* pipe_cmd_create(struct cmd* left, struct cmd* right) {

	if (!right)
		return left;
	
	struct pipecmd* p;

	p = (struct pipecmd*)calloc(sizeof(*p), sizeof(*p));
	
	p->type = PIPE;
	p->leftcmd = left;
	p->rightcmd = right;
	
	return (struct cmd*)p;
}

