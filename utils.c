#include "utils.h"

// splits a string line in two
// acording to the splitter character
char* split_line(char* buf, char splitter) {

	int i = 0;

	while (buf[i] != splitter &&
		buf[i] != END_STRING)
		i++;
		
	buf[i++] = END_STRING;
	
	while (buf[i] == SPACE)
		i++;
	
	return &buf[i];
}

// looks in a block for the 'c' character
// and returns the index in which it is, or -1
// in other case
int block_contains(char* buf, char c) {

	for (int i = 0; i < strlen(buf); i++)
		if (buf[i] == c)
			return i;
	
	return -1;
}

// sets the "key" argument with the key part of
// the "arg" argument and null-terminates it
void get_environ_key(char* arg, char* key) {

	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets the "value" argument with the value part of
// the "arg" argument and null-terminates it
void get_environ_value(char* arg, char* value, int idx) {

	int i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// saves command string representation
// for 'history' command
void save_command(char* scmd) {

	FILE* f;

	if (!(f = fopen(".sh_history", "a+"))) {
		perror("save_command: cannot open file ");
		_exit(EXIT_FAILURE);
	}

	fprintf(f, "%s\n", scmd);

	fclose(f);
}

