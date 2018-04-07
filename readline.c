#include "defs.h"
#include "readline.h"

static char buffer[BUFLEN];

// read a line from the standar input
// and prints the prompt
char* read_line(const char* promt) {

	int i = 0,
	    c = 0;

	memset(buffer, 0, BUFLEN);
	
	// signal handler sets
	// 'background' in true
	if (background) {
		background = 0;
		return buffer;
	}

	fprintf(stdout, "%s %s %s\n", COLOR_RED, promt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}
		
	// if the user press ctrl+D
	// just exit normally
	if (c == EOF && !background)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

