%{

#include <stdio.h>
#include "shl.h"

extern int yylex();
int yyerror(const char* s);

%}

%locations
%pure-parser

%union {
  char *str;
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

%type <str> command_name
%type <node> command
%type <node> pipeline 
%type <node> list 

%%

input
	:
	| input line { }
	;

line
	: T_EOL 
	| list T_EOL { node_execute($1); }
	;

list
	: pipeline { $$ = $1; }
	| list T_AND pipeline
	| list T_OR pipeline
	;

pipeline
	: command { $$ = $1; }
	| pipeline T_PIPE command { $$ = node_pipe($1, $3); }
	;

command
	: command_name { $$ = node_create($1); }
	| command_name args { $$ = node_create($1); } // @todo pass args
	| command T_REDIRECT_IN T_STR { $$ = node_redirect_input($1, $3); }
	| command T_REDIRECT_OUT T_STR { $$ = node_redirect_output($1, $3); }
	;

command_name
	: T_STR
	;

args
	: arg
	| args arg
	;

arg
	: T_STR
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
