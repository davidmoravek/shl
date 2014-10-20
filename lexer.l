%{

#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parser.tab.h"

int read_input(char *buf, size_t max_size);

#define YY_INPUT(buf, result, max_size) result = read_input(buf, max_size);

%}

%x SINGLE_QUOTED
%x DOUBLE_QUOTED

%%

\|                     { return PIPE; }

\n                     { return EOL; }

[']                    { BEGIN(SINGLE_QUOTED); }
<SINGLE_QUOTED>[^']    { }
<SINGLE_QUOTED>[']     { BEGIN(INITIAL); return WORD; }
<SINGLE_QUOTED><<EOF>> { return -1; }

["]                    { BEGIN(DOUBLE_QUOTED); }
<DOUBLE_QUOTED>[^"]    { }
<DOUBLE_QUOTED>["]     { BEGIN(INITIAL); return WORD; }
<DOUBLE_QUOTED><<EOF>> { return -1; }

[a-zA-Z0-9_.-/]+       { return WORD; }

.                      { }

%%

int read_input(char *buf, size_t max_size)
{
	char *line;
	size_t line_size;
	
	//char cwd[1024]; constant size
	//getcwd(cwd, sizeof(cwd));
	//getenv("HOME"); substr and replace with tilde 
	//line = readline(cwd);
	
	line = readline("prikazovatko> ");
	printf("[%s]\n", line);

	if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) 
		return YY_NULL; 

	// filter out empty strings
	if (line[0] != '\0')
		add_history(line);	

	// append EOL
	line_size = strlen(line);
	line[line_size++] = '\n';

	// pass line to parser
	memcpy(buf, line, line_size);

	free(line);
	
	return strlen(buf);
}
