#include "readline.h"

static char buffer[BUFLEN];

static int get_char() {

	char c;
	size_t r;

	r = fread(&c, 1, 1, stdin);

	if (r == 0) {
		if (feof(stdin) > 0)
			c = EOF;
		if (ferror(stdin) > 0)
			c = EINTR;
		clearerr(stdin);
	}

	return c;
}

// read a line from the standar input
// and prints the prompt
char* read_line(const char* promt) {

	int i = 0,
	    c = 0;

	memset(buffer, 0, BUFLEN);
	
	fprintf(stdout, "%s %s %s\n", COLOR_RED, promt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");

	c = get_char();

	while (c != END_LINE && c != EOF && c != EINTR) {
		buffer[i++] = c;
		c = get_char();
	}
	
	// if a background process
	// finishes
	if (c == EINTR)
		return buffer;

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

