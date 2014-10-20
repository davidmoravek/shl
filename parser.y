%{

#include <stdio.h>

extern int yylex();
int yyerror(const char* s);

%}

%token EOL PIPE WORD

%%

input:
	| input line
	;

line:
	command_list EOL { printf("line\n"); }
	;

command:
	WORD { printf("command\n"); }
	;

command_list:
	command { printf("command list\n") }
	| command PIPE command_list { printf("piped\n"); }
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
