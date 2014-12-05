#ifndef SHL_H
#define SHL_H

#include <unistd.h>

typedef enum {
	NODE_AND,
	NODE_BG,
	NODE_OR,
	NODE_PIPE,
	NODE_CMD
} NodeType;

typedef struct arg_t {
	char *name;
	struct arg_t *next;
} Arg;

typedef struct command_t {
	char *name;
	Arg *args;
	char *in;
	char *out;
	int out_append;
} Cmd;

typedef struct node_t {
	NodeType type;
	union {
		struct {
			struct node_t *l;
			struct node_t *r;
		};
		Cmd *cmd;
	};
} Node;

Arg *arg_create(char *);

Cmd *cmd_create(char *, Arg *);

Node *node_and(Node *, Node *);
Node *node_bg(Node *, Node *);
Node *node_cmd(Cmd *);
int node_execute(Node *);
Node *node_or(Node *, Node *);
Node *node_pipe(Node *, Cmd *);

int read_input(char *buf, size_t max_size);

extern int is_multiline;

#endif
