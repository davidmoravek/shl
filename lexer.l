%{

#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shl.h"
#include "parser.tab.h"

static int read_input(char *buf, size_t max_size);

#define YY_INPUT(buf, result, max_size) result = read_input(buf, max_size);

%}

%option bison-bridge

%%

\|                 { return T_PIPE; }
>>                 { return T_APPEND; }
&                  { return T_BACKGROUND; }
&&                 { return T_AND; }
\|\|               { return T_OR; }
;                  { return T_SEMI; }
[a-zA-Z0-9_\.\-/]+ { yylval->str = strdup(yytext); return T_STR; }
\"[^\"]+\"         { yylval->str = strdup(yytext); return T_QSTR; }
=                  { return T_ASSIGN; }
\n                 { return T_EOL; }
.                  { }

%%

static int read_input(char *buf, size_t max_size)
{
	char *line;
	size_t line_size;

	char cwd[1024];
	char prompt[1024 + 32];

	getcwd(cwd, sizeof(cwd));
	sprintf(prompt, "\x1b[31m%s\x1b[0m\nshl > ", cwd);

	line = readline(prompt);

	if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
		return YY_NULL;
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

int yywrap() {
	return 1;
}
