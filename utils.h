#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

char* split_line(char* buf, char splitter);

int block_contains(char* buf, char c);

void get_environ_key(char* arg, char* key);

void get_environ_value(char* arg, char* value, int idx);

void save_command(char* scmd);

#endif // UTILS_H
