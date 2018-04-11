#include "parsing.h"

// parses an argument of the command stream input
static char* get_token(char* buf, int idx) {

	char* tok;
	int i;
	
	tok = (char*)calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		tok[i] = buf[idx];
		i++; idx++;
	}

	return tok;
}

// parses and changes stdin/out/err if needed
static bool parse_redir_flow(struct execcmd* c, char* arg) {

	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
			// stdout redir
			case 0: {
				strcpy(c->out_file, arg + 1);
				break;
			}
			// stderr redir
			case 1: {
				strcpy(c->err_file, &arg[outIdx + 1]);
				break;
			}
		}
		
		free(arg);
		c->type = REDIR;
		
		return true;
	}
	
	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file, arg + 1);
		
		c->type = REDIR;
		free(arg);
		
		return true;
	}
	
	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool parse_environ_var(struct execcmd* c, char* arg) {

	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if (block_contains(arg, '=') > 0) {

		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] = arg;
			return true;
		}
	}
	
	return false;
}

// expand environment variables
static char* expand_environ_var(char* arg) {

	if (arg[0] == '$') {

		char* aux;
		char s[10];

		if (arg[1] == '?') {
			sprintf(s, "%d", status);
			aux = s;
		}
		else
			aux = getenv(arg + 1);

		strcpy(arg, aux);
	}
	
	return arg;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd* parse_exec(char* buf_cmd) {

	struct execcmd* c;
	char* tok;
	int idx = 0, argc = 0;
	
	c = (struct execcmd*)exec_cmd_create(buf_cmd);
	
	while (buf_cmd[idx] != END_STRING) {
	
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);
		
		if (buf_cmd[idx] != END_STRING)
			idx++;
		
		tok = expand_environ_var(tok);
		
		if (parse_redir_flow(c, tok))
			continue;
		
		if (parse_environ_var(c, tok))
			continue;
		
		c->argv[argc++] = tok;
	}
	
	c->argv[argc] = (char*)NULL;
	c->argc = argc;
	
	return (struct cmd*)c;
}

// parses a command knowing that it contains
// the '&' char
static struct cmd* parse_back(char* buf_cmd) {

	int i = 0;
	struct cmd* e;

	while (buf_cmd[i] != '&')
		i++;
	
	buf_cmd[i] = END_STRING;
	
	e = parse_exec(buf_cmd);

	return back_cmd_create(e);
}

// parses a command and checks if it contains
// the '&' (background process) character
static struct cmd* parse_cmd(char* buf_cmd) {

	if (strlen(buf_cmd) == 0)
		return NULL;
		
	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 &&
			buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);
		
	return parse_exec(buf_cmd);
}

// parses the command line recursively
// looking for the pipe character '|'
struct cmd* parse_line(char* buf) {
	
	struct cmd *r = NULL, *l;
	
	char* right = split_line(buf, '|');
	
	if (strlen(right) != 0)
		r = parse_line(right);

	l = parse_cmd(buf);
	
	return pipe_cmd_create(l, r);
}
