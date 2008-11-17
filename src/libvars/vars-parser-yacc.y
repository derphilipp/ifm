%{
/* Expression parser */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "vars-buffer.h"
#include "vars-hash.h"
#include "vars-parser.h"
#include "vars-parser-lex.h"
#include "vars-parser-yacc.h"
#include "vars-regex.h"

/* Convenience macros */
#define COPY(s)  vs_copy(s)
#define DSET(s)  vs_dcreate(s)
#define DVAL(s)  vs_dget(s)
#define ISET(s)  vs_icreate(s)
#define IVAL(s)  vs_iget(s)
#define PUSH(s)  vl_push(vp_stack, s)
#define STR(s)   (vs_type(s) == V_TYPE_STRING)
#define SVAL(s)  vs_sgetref(s)

#define YYPRINT(fp, type, value) \
        yyprint(fp, type, value)

#define YYERROR_VERBOSE 1
#define YYDEBUG 1

/* Current parser for evaluation */
vparser *vp_parser = NULL;

/* Current value stack */
static vlist *vp_stack = NULL;

/* Current function arg list */
static vlist *vp_arglist = NULL;

/* Internal arg list */
static vlist *args = NULL;

/* Internal functions */
static vregex *get_regexp(char *str);
static void yyprint(FILE *fp, int type, vscalar *s);
void yyerror(char *msg);

%}

%token          VALUE ID
%token          EQ NE GE GT LE LT MAT NMAT
%token          AND OR

%left           ';'
%right		'='
%right		'?' ':'
%left		OR
%left		AND
%left		EQ NE MAT NMAT
%left		LT LE GT GE
%left		'+' '-'
%left		'*' '/' '%'
%left		PLUS MINUS
%nonassoc	'!'
%right		'^'

%%

input           : /* empty */
                | input line
                ;

line            : '\n'
                | exp '\n'
                | error '\n'
                {
                    yyerrok;
                }
                ;

exp             : VALUE
                {
                    PUSH($1);
                }
                | ID
                {
                    $$ = vp_get(vp_parser, SVAL($1));

                    if ($$ != NULL)
                        $$ = COPY($$);
                    else
                        $$ = vs_create(V_TYPE_UNDEF);

                    PUSH($1);
                    PUSH($$);
                }
                | '+' exp %prec PLUS
                {
                    $$ = DSET(DVAL($2)); PUSH($$);
                }
                | '-' exp %prec MINUS
                {
                    $$ = DSET(-DVAL($2)); PUSH($$);
                }
                | exp '+' exp
                {
                    $$ = DSET(DVAL($1) + DVAL($3)); PUSH($$);
                }
                | exp '-' exp
                {
                    $$ = DSET(DVAL($1) - DVAL($3)); PUSH($$);
                }
                | exp '*' exp
                {
                    $$ = DSET(DVAL($1) * DVAL($3)); PUSH($$);
                }
                | exp '/' exp
                {
                    $$ = DSET(DVAL($1) / DVAL($3)); PUSH($$);
                }
                | exp '^' exp
                {
                    errno = 0;
                    $$ = DSET(pow(DVAL($1), DVAL($3))); PUSH($$);
                    if (errno)
                        vp_errno("^", errno);
                }
                | exp '%' exp
                {
                    errno = 0;
                    $$ = DSET(fmod(DVAL($1), DVAL($3))); PUSH($$);
                    if (errno)
                        vp_errno("%", errno);
                }
                | exp ';' exp
                {
                    $$ = COPY($3); PUSH($$);
                }
                | '(' exp ')'
                {
                    $$ = COPY($2); PUSH($$);
                }
                | exp EQ exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) == 0);
                    else
                        $$ = ISET(DVAL($1) == DVAL($3));

                    PUSH($$);
                }
                | exp NE exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) != 0);
                    else
                        $$ = ISET(DVAL($1) != DVAL($3));

                    PUSH($$);
                }
                | exp GT exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) > 0);
                    else
                        $$ = ISET(DVAL($1) > DVAL($3));

                    PUSH($$);
                }
                | exp LT exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) < 0);
                    else
                        $$ = ISET(DVAL($1) < DVAL($3));

                    PUSH($$);
                }
                | exp GE exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) >= 0);
                    else
                        $$ = ISET(DVAL($1) >= DVAL($3));

                    PUSH($$);
                }
                | exp LE exp
                {
                    if (STR($1) && STR($3))
                        $$ = ISET(strcmp(SVAL($1), SVAL($3)) <= 0);
                    else
                        $$ = ISET(DVAL($1) <= DVAL($3));

                    PUSH($$);
                }
                | exp AND exp
                {
                    $$ = ISET(IVAL($1) && IVAL($3)); PUSH($$);
                }
                | exp OR exp
                {
                    $$ = ISET(IVAL($1) || IVAL($3)); PUSH($$);
                }
                | exp MAT exp
                {
                    vregex *r = get_regexp(SVAL($3));
                    $$ = ISET(r != NULL ? vr_match(SVAL($1), r) : 0);
                    PUSH($$);
                }
                | exp NMAT exp
                {
                    vregex *r = get_regexp(SVAL($3));
                    $$ = ISET(r != NULL ? !vr_match(SVAL($1), r) : 0);
                    PUSH($$);
                }
                | '!' exp
                {
                    $$ = ISET(!IVAL($2)); PUSH($$);
                }
                | exp '?' exp ':' exp
                {
                    $$ = COPY(IVAL($1) ? $3 : $5); PUSH($$);
                }
                | ID '=' exp
                {
                    $$ = COPY($3);
                    PUSH($1);
                    PUSH($$);
                    vp_store(vp_parser, SVAL($1), COPY($3));
                }
                | ID '('
                {
                    PUSH($1);
                    if (args != NULL)
                        vl_ppush(vp_arglist, args);
                    args = vl_create();
                }
                arglist ')'
                {
                    if (($$ = vp_call(vp_parser, SVAL($1), args)) == NULL)
                        $$ = vs_create(V_TYPE_UNDEF);
                    PUSH($$);
                    vl_destroy(args);
                    args = vl_ppop(vp_arglist);
                }
                ;

arglist         : /* empty */
                | arg
                | arglist ',' arg
                ;

arg             : exp
                {
                    vl_push(args, COPY($1));
                }
                ;

%%

/* Parse current input */
vscalar *
vp_parse(vparser *p, char *expr)
{
    vscalar *retval;
    V_BUF_DECL;

    /* Initialize */
    vp_parser = p;

    /* Add final newline */
    V_BUF_SET(expr);
    V_BUF_ADD("\n");

    /* Clear stack */
    vl_init(vp_stack);

    /* Create argument list */
    if (vp_arglist == NULL)
        vp_arglist = vl_create();

    /* Let's parse! */
    vp_scan_start(V_BUF_VAL);
    yyparse();
    vp_scan_finish();

    /* Get return value */
    if (vl_length(vp_stack) > 0)
        retval = vl_pop(vp_stack);
    else
        retval = vs_create(V_TYPE_UNDEF);

    /* Clean up */
    vp_parser = NULL;

    return retval;
}

/* Get a cached regexp */
static vregex *
get_regexp(char *str)
{
    static vhash *cache = NULL;
    vregex *r = NULL;

    if (cache == NULL)
        cache = vh_create();

    if ((r = vh_pget(cache, str)) != NULL)
        return r;

    if ((r = vr_create(str)) != NULL)
        vh_pstore(cache, str, r);
    else
        vp_err("invalid regexp: %s", str);

    return r;
}

/* Parser print function */
static void
yyprint(FILE *fp, int type, vscalar *s)
{
    switch (type) {
    case ID:
    case VALUE:
        fprintf(fp, " %s", vs_sget(s));
        break;
    }
}

/* Parser error function */
void
yyerror(char *msg)
{
    vp_err(msg);
}
