#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shl.h"

static int process_execute(Node *node);
static int process_command(Node *node);
static int process_pipe(Node *node);
static int wait_for_child(pid_t);

Node *node_create(char *command) {
	Node *node = (Node *) malloc(sizeof(Node));
	node->type = NODE_COMMAND;
	node->command = command;
	node->next = NULL;
	return node;
}

int node_execute(Node *node) {
	int status = process_execute(node);
	free(node);
	return status;
}

Node *node_pipe(Node *left, Node *right) {
	Node *most_right = left;
	while (most_right->next != NULL)
		most_right = most_right->next;
	most_right->next = right;
	return left;
}

Node *node_redirect_input(Node *node, char *input) {
	return NULL;
}

Node *node_redirect_output(Node *node, char *output) {
	return NULL;
}

static int process_execute(Node *node) {
	if (node == NULL)
		return 0;

	switch (node->type) {
		case NODE_COMMAND:
			return process_command(node);
		case NODE_PIPE:
			return process_pipe(node);
		default:
			return 1;
	}
}

static int process_command(Node *node) {
	pid_t child;
	if ((child = fork()) == 0) {
		char **argv = (char **) malloc(2 * sizeof(char *));
		argv[0] = node->command;
		argv[1] = NULL;
		execvp(node->command, argv);
		perror(node->command);
		free(argv);
		exit(1);
	} else {
		return wait_for_child(child);
	}
	return 1;
}

static int process_pipe(Node *node) {
	return 1;
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
