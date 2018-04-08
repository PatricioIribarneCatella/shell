#ifndef RUNCMD_H
#define RUNCMD_H

#include "defs.h"
#include "parsing.h"
#include "exec.h"
#include "printstatus.h"
#include "freecmd.h"
#include "builtin.h"

extern pid_t back;
extern struct cmd* parsed_pipe;
extern struct cmd* parsed_back;
extern char back_cmd[BUFLEN];

int run_cmd(char* cmd);

#endif // RUNCMD_H
