#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shl.h"

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
 * EVAL
 */

static int do_command(Node *);
static int do_pipe(Node *);
static void redirect_stream(int, int);
static int wait_for_child(pid_t);

static int do_execute(Node *node) {
	if (node == NULL) {
		return 0;
	}
	switch (node->type) {
		case NODE_CMD:
			return do_command(node);
		case NODE_PIPE:
			return do_pipe(node);
		default:
			return 1;
	}
}

static int do_command(Node *node) {
	Cmd *cmd = node->cmd;
	pid_t child;
	if ((child = fork()) == 0) {
		char **argv = arg_vector(cmd);
		execvp(cmd->name, argv);
		perror(cmd->name);
		free(argv);
		exit(1);
	} else {
		return wait_for_child(child);
	}
	return 1;
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
