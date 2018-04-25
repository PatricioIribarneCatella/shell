#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"
#include "utils.h"

extern char promt[PRMTLEN];
extern int status;

int cd(char* cmd);

int pwd(char* cmd);

int export_var(char* cmd);

int exit_shell(char* cmd);

#endif // BUILTIN_H
