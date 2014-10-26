typedef enum {
	NODE_COMMAND,
	NODE_AND,
	NODE_OR,
	NODE_PIPE
} NodeType;

typedef struct node_t {
	NodeType type;
	char *command;
	// variables
	// arguments
	struct node_t *next;
} Node;

Node *node_create(char *);
int node_execute(Node *);
Node *node_pipe(Node *, Node *);
Node *node_redirect_input(Node *, char *);
Node *node_redirect_output(Node *, char *);
