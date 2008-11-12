/* Expression scanner header */

#ifndef VARS_PARSER_LEX_H
#define VARS_PARSER_LEX_H

#include "vars-scalar.h"

#define YYSTYPE vscalar *

/* Internalize yacc variables */
#ifdef yylex
#undef yylex
#endif

#define	yymaxdepth       parser_maxdepth
#define	yyparse          parser_parse
#define	yylex            parser_lex
#define	yyerror          parser_error
#define	yylval           parser_lval
#define	yychar           parser_char
#define	yydebug          parser_debug
#define	yypact           parser_pact
#define	yyr1             parser_r1
#define	yyr2             parser_r2
#define	yydef            parser_def
#define	yychk            parser_chk
#define	yypgo            parser_pgo
#define	yyact            parser_act
#define	yyexca           parser_exca
#define yyerrflag        parser_errflag
#define yynerrs          parser_nerrs
#define	yyps             parser_ps
#define	yypv             parser_pv
#define	yys              parser_s
#define	yy_yys           parser_yys
#define	yystate          parser_state
#define	yytmp            parser_tmp
#define	yyv              parser_v
#define	yy_yyv           parser_yyv
#define	yyval            parser_val
#define	yylloc           parser_lloc
#define yyreds           parser_reds
#define yytoks           parser_toks
#define yylhs            parser_yylhs
#define yylen            parser_yylen
#define yydefred         parser_yydefred
#define yydgoto          parser_yydgoto
#define yysindex         parser_yysindex
#define yyrindex         parser_yyrindex
#define yygindex         parser_yygindex
#define yytable          parser_yytable
#define yycheck          parser_yycheck
#define yyname           parser_yyname
#define yyrule           parser_yyrule

extern int scanner_line;

extern void vp_scan_start(char *expr);
extern void vp_scan_finish(void);

#endif
