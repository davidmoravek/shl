#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shl.h"

#define MAX_JOBS 32

int is_multiline = 0;

/**
 * READ
 */

int read_input(char *buf, size_t max_size)
{
	char *line;
	size_t line_size;
	char prompt[1024 + 32];
	if (is_multiline) {
		sprintf(prompt, "> ");
	} else {
		char cwd[1024];
		char *home = getenv("HOME");
		getcwd(cwd, sizeof(cwd));
		if (strncmp(home, &cwd[0], strlen(home)) == 0) {
			sprintf(prompt, "\x1b[31m~%s\x1b[0m\n$ ", &cwd[strlen(home)]);
		} else {
			sprintf(prompt, "\x1b[31m%s\x1b[0m\n$ ", cwd);
		}
	}
	line = readline(prompt);
	if (line == NULL) {
		exit(0);
	}
	if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
		return 0;
	}
	// filter out empty strings
	if (line[0] != '\0') {
		add_history(line);
	}
	line_size = strlen(line);
	memcpy(buf, line, line_size);
	// append EOL and terminating NULL byte
	buf[line_size] = '\n';
	buf[line_size + 1] = '\0';
	free(line);
	return strlen(buf);
}

/**
 * ARGS
 */

Arg *arg_create(char *name) {
	Arg *arg = (Arg *) malloc(sizeof(Arg));
	arg->name = name;
	arg->next = NULL;
	return arg;
}

static int arg_count(Arg *arg) {
	Arg *temp = arg;
	int cnt = 0;
	while (temp) {
		cnt++;
		temp = temp->next;
	}
	return cnt;
}

static Arg *arg_reverse(Arg *head) {
	Arg *prev = NULL;
	while (head) {
		Arg *temp = head;
		head = head->next;
		temp->next = prev;
		prev = temp;
	}
	return prev;
}

static char **arg_vector(Cmd *cmd) {
	int argc = 2 + arg_count(cmd->args);
	char **argv = (char **) malloc(argc * sizeof(char *));
	argv[0] = cmd->name;
	argv[argc - 1] = NULL;
	if (argc > 2) {
		Arg *temp = cmd->args;
		for (int i = 1; temp != NULL; i++, temp = temp->next)
			argv[i] = temp->name;
	}
	return argv;
}

/**
 * CMDS
 */

Cmd *cmd_create(char *name, Arg * args) {
	Cmd *cmd = (Cmd *) malloc(sizeof(Cmd));
	cmd->name = name;
	cmd->args = args == NULL ? args : arg_reverse(args);
	cmd->in = NULL;
	cmd->out = NULL;
	cmd->out_append = 0;
	return cmd;
}

/**
 * NODES
 */

static int do_execute(Node *);

static Node *node_create(NodeType t) {
	Node *node = (Node *) malloc(sizeof(Node));
	node->type = t;
	return node;
}

int node_execute(Node *node) {
	int status = do_execute(node);
	free(node); // @todo recursive
	return status;
}

Node *node_cmd(Cmd *cmd) {
	Node *node = node_create(NODE_CMD);
	node->cmd = cmd;
	return node;
}

Node *node_and(Node *l, Node *r) {
	Node *node = node_create(NODE_AND);
	node->l = l;
	node->r = r;
	return node;
}

Node *node_bg(Node *l, Node *r) {
	Node *node = node_create(NODE_BG);
	node->l = l;
	node->r = r;
	return node;
}

Node *node_or(Node *l, Node *r) {
	Node *node = node_create(NODE_OR);
	node->l = l;
	node->r = r;
	return node;
}

Node *node_pipe(Node *l, Cmd * r) {
	Node *node = node_create(NODE_PIPE);
	node->l = l;
	node->r = node_cmd(r);
	return node;
}

/**
 * BUILTINS
 */

static int builtin_cd(Cmd *cmd) {
	int argc = arg_count(cmd->args);
	char *p;

	if (argc > 1) {
		printf("cd: Expects only one argument\n");
		return 1;
	}

	if (argc == 0) {
		// no arguments ... enter home dir
		p = getenv("HOME");
		if (p == NULL) {
			printf("cd: undefined HOME\n");
			return 1;
		}
	} else {
		p = cmd->args->name;
	}

	if (chdir(p) != 0) {
		perror("cd");
		return 1;
	}

	return 0;
}

/**
 * EVAL
 */

static int do_bg(Node *);
static int do_cmd(Cmd *);
static int do_chain(Node *);
static int do_pipe(Node *);
static void redirect_stream(int, int);
static int wait_for_child(pid_t);

static int do_execute(Node *node) {
	if (node == NULL) {
		return 0;
	}
	switch (node->type) {
		case NODE_BG:
			return do_bg(node);
		case NODE_AND:
		case NODE_OR:
			return do_chain(node);
		case NODE_CMD:
			return do_cmd(node->cmd);
		case NODE_PIPE:
			return do_pipe(node);
		default:
			return 1;
	}
}

static int do_bg(Node *node) {
	pid_t child;
	if ((child = fork()) == 0) {
		exit(do_execute(node->l));
	} else {
		return do_execute(node->r);
	}
}

static int do_chain(Node *node) {
	int s, do_next;
	s = do_execute(node->l);
	if (node->type == NODE_AND) {
		do_next = !s;
	} else { // OR
		do_next = s;
	}
	return do_next ? do_execute(node->r) : s;
}

static int do_cmd(Cmd *cmd) {
	pid_t child;

	if (strcmp(cmd->name, "cd") == 0) {
		return builtin_cd(cmd);
	}

	if ((child = fork()) == 0) {
      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      signal (SIGCHLD, SIG_DFL);
		char **argv = arg_vector(cmd);
		if (cmd->in != NULL) {
			int fd = open(cmd->in, O_RDONLY);
			if (fd < 0) {
				perror("input file");
			} else {
				redirect_stream(fd, STDIN_FILENO);
			}
		}
		if (cmd->out != NULL) {
			int fd = open(cmd->out, O_WRONLY | O_CREAT | (cmd->out_append ? O_APPEND : O_TRUNC), 0666);
			if (fd < 0) {
				perror("output file");
			} else {
				redirect_stream(fd, STDOUT_FILENO);
			}
		}
		execvp(cmd->name, argv);
		perror(cmd->name);
		free(argv);
		exit(1);
	} else {
		return wait_for_child(child);
	}
}

static int do_pipe(Node *node) {
	int pipefd[2];
	if (pipe(pipefd) == -1) {
		perror("pipe");
		return 1;
	}
	int readfd = pipefd[0];
	int writefd = pipefd[1];
	pid_t source;
	pid_t target;
	if ((source = fork()) == 0) {
		int s;
		close(readfd);
		redirect_stream(writefd, STDOUT_FILENO);
		s = do_execute(node->l);
		close(writefd);
		exit(s);
	} else {
		close(writefd);
		if ((target = fork()) == 0) {
			int s;
			redirect_stream(readfd, STDIN_FILENO);
			s = do_execute(node->r);
			close(readfd);
			exit(s);
		} else {
			close(readfd);
			wait_for_child(source);
			return wait_for_child(target);
		}
	}
}

static void redirect_stream(sourcefd, targetfd) {
	if (dup2(sourcefd, targetfd) == -1) {
		perror("dup2");
		return;
	}
	if (close(sourcefd) == -1) {
		perror("close");
	}
}

/**
 * @see http://linux.die.net/man/2/waitpid
 */
static int wait_for_child(pid_t pid) {
	int status;
	pid_t r = waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	return 1;
}
