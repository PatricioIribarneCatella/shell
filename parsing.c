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
static bool parse_redir_flow(struct redircmd* r, char* arg) {

	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {

		r->out.flags = O_WRONLY | O_CREAT;
		r->err.flags = O_WRONLY | O_CREAT | O_TRUNC;

		switch (outIdx) {
			// stdout redir
			case 0: {
				if (arg[++outIdx] == '>') {
					r->out.flags |= O_APPEND;
					strcpy(r->out.name, arg + 2);
				} else {
					r->out.flags |= O_TRUNC;
					strcpy(r->out.name, arg + 1);
				}
				break;
			}
			// stderr redir
			case 1: {
				strcpy(r->err.name, &arg[outIdx + 1]);
				break;
			}
		}
		
		free(arg);
		
		return true;
	}
	
	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		
		r->in.flags = O_RDONLY;

		strcpy(r->in.name, arg + 1);

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
	
	char* aux;
	char s[10];
	size_t len;

	if (arg[0] == '$') {
		
		if (arg[1] == '?') {
			sprintf(s, "%d", status);
			aux = s;
		}
		else
			aux = getenv(arg + 1);

		if (!aux)
			strcpy(arg, " ");
		else {
			len = strlen(aux);

			if (len > ARGSIZE)
				arg = realloc(arg, (len + 1) * sizeof(char));
			
			strcpy(arg, aux);
		}
	}
	
	return arg;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static void parse_exec(char* buf_cmd, struct redircmd* r) {

	char* tok;
	struct execcmd* e = (struct execcmd*)(r->c);
	int idx = 0, argc = 0;
	
	while (buf_cmd[idx] != END_STRING) {
	
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);
		
		if (buf_cmd[idx] != END_STRING)
			idx++;
		
		if (parse_redir_flow(r, tok))
			continue;
		
		if (parse_environ_var(e, tok))
			continue;
		
		tok = expand_environ_var(tok);
		
		e->argv[argc++] = tok;
	}
	
	e->argv[argc] = (char*)NULL;
	e->argc = argc;
}

static struct cmd* parse_redir(char* buf_cmd) {

	struct redircmd* r;

	r = (struct redircmd*)redir_cmd_create(buf_cmd);

	parse_exec(buf_cmd, r);

	return (struct cmd*)r;
}

// parses a command knowing that it contains
// the '&' char
static struct cmd* parse_back(char* buf_cmd) {

	int i = 0;
	struct cmd* e;

	while (buf_cmd[i] != '&')
		i++;
	
	buf_cmd[i] = END_STRING;
	
	e = parse_redir(buf_cmd);

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
		
	return parse_redir(buf_cmd);
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

