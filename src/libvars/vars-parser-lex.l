%{
/* Expression scanner */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "vars-buffer.h"
#include "vars-config.h"
#include "vars-parser-lex.h"
#include "vars-parser-yacc.h"

#define parserwrap() 1

static YY_BUFFER_STATE string;
int scanner_line = 0;
static vbuffer *sbuf = NULL;

%}

DIGIT	[0-9]
DECIMAL \.{DIGIT}*
EXP	[Ee][-+]?{DIGIT}+
LETTER	[A-Za-z_]
ALNUM	[A-Za-z_0-9]
REAL	{DIGIT}*{DECIMAL}

%%

"&&"|"and"  { return AND; }
"||"|"or"   { return OR; }

"=="|"eq"   { return EQ; }
"!="|"ne"   { return NE; }

">="|"ge"   { return GE; }
">"|"gt"    { return GT; }

"<="|"le"   { return LE; }
"<"|"lt"    { return LT; }

"=~"|"mat"  { return MAT; } 
"!~"|"nmat" { return NMAT; } 

{REAL}        |
{REAL}?{EXP}  |
{DIGIT}+{EXP} {
    /* Real number */
    yylval = vs_dcreate(atof(yytext));
    return VALUE;
} 

{DIGIT}+ {
    /* Integer */
    yylval = vs_icreate(atoi(yytext));
    return VALUE;
}

{LETTER}{ALNUM}* {
    /* Variable */
    yylval = vs_screate(yytext);
    return ID;
}

\" {
    /* Quoted string */
    int c, quote = 0;

    /* Initialise */
    vb_init(sbuf);

    /* Slurp string in, one char at a time */
    while (1) {
        c = input();

        if        (c == EOF) {
            yyerror("unterminated string");
            break;
        } else if (c == '"' && !quote) {
            break;
        } else if (c == '\\' && !quote) {
            quote = 1;
            continue;
        } else {
            vb_putc(sbuf, c);
            if (c == '\n')
                scanner_line++;
        }

        quote = 0;
    }

    yylval = vs_screate(vb_get(sbuf));
    return VALUE;
}

#.* {
    /* Comment */
}

[ \t\f\r]+ {
    /* Whitespace */
}

\n {
    /* Newline */
    scanner_line++;
    return yytext[0];
}

. {
    /* Single character */
    return yytext[0];
}

%%

void
vp_scan_start(char *expr)
{
    scanner_line = 0;
    string = yy_scan_string(expr);
}

void
vp_scan_finish(void)
{
    yy_delete_buffer(string);
}

/*
  Local Variables:
  mode:c
  End:
 */
