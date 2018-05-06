#include "createcmd.h"

// creates an execcmd struct to store 
// the args and environ vars of the command
struct cmd* exec_cmd_create(char* buf_cmd) {

	struct execcmd* e;
	
	e = (struct execcmd*)calloc(sizeof(*e), sizeof(*e));

	e->type = EXEC;
	strcpy(e->scmd, buf_cmd);
	
	return (struct cmd*)e;
}

struct cmd* redir_cmd_create(char* buf_cmd) {

	struct redircmd* r;

	r = (struct redircmd*)calloc(sizeof(*r), sizeof(*r));

	r->type = REDIR;
	r->c = exec_cmd_create(buf_cmd);
	strcpy(r->scmd, buf_cmd);

	return (struct cmd*)r;
}

// creates a backcmd struct to store the
// background command to be executed
struct cmd* back_cmd_create(struct cmd* c) {

	struct backcmd* b;

	b = (struct backcmd*)calloc(sizeof(*b), sizeof(*b));
	
	b->type = BACK;
	strcpy(b->scmd, c->scmd);
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

