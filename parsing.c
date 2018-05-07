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

// parse flow redirection for output
static bool parse_redir_output(char* tok,
		struct file* out, struct file* err) {

	int outIdx;

	if ((outIdx = block_contains(tok, '>')) >= 0) {

		out->flags = O_WRONLY | O_CREAT;
		err->flags = O_WRONLY | O_CREAT | O_TRUNC;

		switch (outIdx) {
			// stdout redir
			case 0: {
				if (tok[++outIdx] == '>') {
					out->flags |= O_APPEND;
					strcpy(out->name, tok + 2);
				} else {
					out->flags |= O_TRUNC;
					strcpy(out->name, tok + 1);
				}
				break;
			}
			// stderr redir
			case 1: {
				strcpy(err->name, &tok[outIdx + 1]);
				break;
			}
		}
		
		free(tok);
		return true;
	}

	return false;
}

// parse flow redirection for input
static bool parse_redir_input(char* tok, struct file* in) {

	int inIdx;

	if ((inIdx = block_contains(tok, '<')) >= 0) {
		// stdin redir
		in->flags = O_RDONLY;

		strcpy(in->name, tok + 1);

		free(tok);
		return true;
	}

	return false;
}

// parses and changes stdin/out/err if needed
static void parse_redir_flow(char* buf,
			struct file* in,
			struct file* out,
			struct file* err) {

	int idx = 0;
	char* tok;
	
	while (buf[idx] != END_STRING) {

		tok = get_token(buf, idx);
		idx = idx + strlen(tok);

		if (buf[idx] != END_STRING)
			idx++;

		if (parse_redir_output(tok, out, err))
			continue;

		if (parse_redir_input(tok, in))
			continue;
		
		free(tok);
	}
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

// checks if the token contains
// a redir operator
static bool contains_redir(char* arg) {

	if (block_contains(arg, '>') >= 0 ||
		block_contains(arg, '<') >= 0) {
		
		free(arg);
		return true;
	}

	return false;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd* parse_exec(char* buf_cmd) {

	char* tok;
	struct execcmd* e;
	int idx = 0, argc = 0;

	e = (struct execcmd*)exec_cmd_create(buf_cmd);
	
	while (buf_cmd[idx] != END_STRING) {
	
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);
		
		if (buf_cmd[idx] != END_STRING)
			idx++;
		
		if (contains_redir(tok))
			continue;
		
		if (parse_environ_var(e, tok))
			continue;
		
		tok = expand_environ_var(tok);
		
		e->argv[argc++] = tok;
	}
	
	e->argv[argc] = (char*)NULL;
	e->argc = argc;

	return (struct cmd*)e;
}

static struct cmd* parse_redir(char* buf_cmd) {

	struct cmd* e;
	struct file in, out, err;

	memset(&in, 0, sizeof(struct file));
	memset(&out, 0, sizeof(struct file));
	memset(&err, 0, sizeof(struct file));
	
	e = parse_exec(buf_cmd);

	parse_redir_flow(buf_cmd, &in, &out, &err);

	return redir_cmd_create(e, &in, &out, &err);
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

