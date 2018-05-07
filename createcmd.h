#ifndef CREATECMD_H
#define CREATECMD_H

#include "defs.h"
#include "types.h"

struct cmd* exec_cmd_create(char* cmd);

struct cmd* redir_cmd_create(struct cmd* e,
	struct file* in, struct file* out, struct file* err);

struct cmd* back_cmd_create(struct cmd* c);

struct cmd* pipe_cmd_create(struct cmd* l, struct cmd* r);

#endif // CREATECMD_H
