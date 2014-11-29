%{

#include <stdio.h>
#include "shl.h"

extern int yylex();
int yyerror(const char* s);

%}

%pure-parser

%union {
	char *str;
	Arg *arg;
	Cmd *cmd;
	Node *node;
}

%token <str> T_STR
%token <str> T_QSTR

%token T_PIPE
%token T_APPEND
%token T_REDIRECT_IN
%token T_REDIRECT_OUT
%token T_BACKGROUND

%token T_AND
%token T_OR
%token T_SEMI

%token T_ASSIGN
%token T_EOL

%type <arg> args
%type <cmd> command
%type <node> pipeline
%type <node> list

%%

input
	:
	| input line
	;

line
	: T_EOL
	| list T_EOL                   { node_execute($1); }
	| list T_SEMI                  { node_execute($1); }
	;

list
	: pipeline                     { $$ = $1; }
	| list T_AND pipeline          { $$ = node_and($1, $3); }
	| list T_OR pipeline           { $$ = node_or($1, $3); }
	;

pipeline
	: command                      { $$ = node_cmd($1); }
	| pipeline T_PIPE command      { $$ = node_pipe($1, $3); }
	;

command
	: T_STR                        { $$ = cmd_create($1, NULL); }
	| T_STR args                   { $$ = cmd_create($1, $2); }
	| command T_REDIRECT_IN T_STR  { $1->in = $3; $$ = $1; }
	| command T_REDIRECT_OUT T_STR { $1->out = $3; $$ = $1; }
	;

args
	: T_STR                        { $$ = arg_create($1); }
	| args T_STR                   { $$ = arg_create($2); $$->next = $1; }
	;

%%

int main(int argc, char *argv[]) {
	yyparse();
	return 0;
}

int yyerror(const char *s)
{
	printf("error: %s\n", s);
	return 0;
}
