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

%token <str> T_STR T_QSTR

%token T_PIPE T_APPEND T_REDIRECT_IN T_REDIRECT_OUT T_BG T_AND T_OR T_SEMI T_ASSIGN T_EOL

%type <arg> args
%type <str> arg_name
%type <cmd> command
%type <node> pipeline list

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
	| list T_BG                    { $$ = node_bg($1, NULL) }
	| list T_BG pipeline           { $$ = node_bg($1, $3); }
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
	| command T_APPEND T_STR       { $1->out = $3; $1->out_append = 1; $$ = $1; }
	;

args
	: arg_name { $$ = arg_create($1); }
	| args arg_name{ $$ = arg_create($2); $$->next = $1; }
	;

arg_name
	: T_STR
	| T_QSTR
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
