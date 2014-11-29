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

/**
 * CMDS
 */

static int do_execute(Cmd *);

Cmd *cmd_create(char *name, Arg * args) {
	if (args != NULL) {
		args = arg_reverse(args);
	}
	Cmd *cmd = (Cmd *) malloc(sizeof(Cmd));
	cmd->name = name;
	cmd->args = args;
	cmd->in = NULL;
	cmd->out = NULL;
	cmd->chain = CHAIN_NONE;
	cmd->next = NULL;
	return cmd;
}

int cmd_execute(Cmd *cmd) {
	int status = do_execute(cmd);
	free(cmd); // @todo recursive
	return status;
}

Cmd *cmd_pipe(Cmd *l, Cmd *r) {
	Cmd *last = l;
	for (; last->next != NULL; last = last->next)
		;
	last->chain = CHAIN_PIPE;
	last->next = r;
	return l;
}

/**
 * EVAL
 */

static int do_command(Cmd *);
static int do_pipe(Cmd *);
static int wait_for_child(pid_t);

static int do_execute(Cmd *cmd) {
	if (cmd == NULL) {
		return 0;
	}

	switch (cmd->chain) {
		case CHAIN_NONE:
			return do_command(cmd);
		case CHAIN_PIPE:
			return do_pipe(cmd);
		default:
			return 1;
	}
}

static int do_command(Cmd *cmd) {
	pid_t child;
	if ((child = fork()) == 0) {
		int argc = 2 + arg_count(cmd->args);
		char **argv = (char **) malloc(argc * sizeof(char *));
		argv[0] = cmd->name;
		argv[argc - 1] = NULL;
		if (argc > 2) {
			int i = 1;
			Arg *temp = cmd->args;
			for (; temp != NULL; temp = temp->next)
				argv[i++] = temp ->name;
		}
		execvp(cmd->name, argv);
		perror(cmd->name);
		free(argv);
		exit(1);
	} else {
		return wait_for_child(child);
	}
	return 1;
}

static int do_pipe(Cmd *cmd) {
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
