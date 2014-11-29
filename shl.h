typedef enum {
	CHAIN_AND,
	CHAIN_OR,
	CHAIN_PIPE,
	CHAIN_NONE
} Chain;

typedef struct arg_t {
	char *name;
	struct arg_t *next;
} Arg;

typedef struct command_t {
	char *name;
	Arg *args;
	char *in;
	char *out;
	Chain chain;
	struct command_t *next;
} Cmd;

Arg *arg_create(char *);
Cmd *cmd_create(char *, Arg *);
int cmd_execute(Cmd *);
Cmd *cmd_pipe(Cmd *, Cmd*);
