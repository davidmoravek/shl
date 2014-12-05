%{

#include <string.h>
#include "shl.h"
#include "parser.tab.h"

#define YY_INPUT(buf, result, max_size) result = read_input(buf, max_size);

%}

%option bison-bridge

%x QUOTED

%%

\|                 { return T_PIPE; }
>>                 { return T_APPEND; }
&                  { return T_BG; }
&&                 { return T_AND; }
\|\|               { return T_OR; }
;                  { return T_SEMI; }
\>                 { return T_REDIRECT_OUT; }
\<                 { return T_REDIRECT_IN; }
[a-zA-Z0-9_\.\-/*]+ { yylval->str = strdup(yytext); return T_STR; }

\"                 { is_multiline = 1; BEGIN QUOTED; }

<QUOTED>[^"]+      { yylval->str = strdup(yytext); return T_QSTR; }
<QUOTED>\"         { is_multiline = 0; BEGIN INITIAL; }


=                  { return T_ASSIGN; }
\n                 { return T_EOL; }
.                  { }

%%


int yywrap() {
	return 1;
}
