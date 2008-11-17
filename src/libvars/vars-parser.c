/*!
  @defgroup parser Parsers
  @ingroup types

  A parser is an object which is used to evaluate mathematical expressions.
  It stores variables, constants and functions which are used to perform
  the evaluations.  Parsers get created with a set of builtin functions and
  constants, and you can define your own or override the builtin ones.
*/

/*!
  @defgroup parser_create Creating and destroying parsers
  @ingroup parser
*/

/*!
  @defgroup parser_exp Parsing expressions
  @ingroup parser
*/

/*!
  @defgroup parser_custom Customizing parsers
  @ingroup parser
*/

/*!
  @defgroup parser_access Accessing parser internals
  @ingroup parser
*/

/*!
  @defgroup parser_syntax Parser input syntax
  @ingroup parser

  Input to a parser consists of a number of expressions. Each expression is
  terminated by a newline.  Whitespace is ignored.  Comments may also be
  included, starting with \c # and continuing to the end of the line.

  Expressions may have a numeric or string value.  Conversion between
  numbers and strings is done automatically whenever required.

  The following types of expression are allowed (where \c exp represents
  another expression):

  - <tt>number</tt> -- A number, with optional decimal part and exponent.

  - <tt>string</tt> -- A string, in double-quotes.  A backslash in a string
    quotes the next character, in the same way as a C string.

  - <tt>exp + exp, exp - exp, exp * exp, exp / exp</tt> -- The four
    standard arithmetic operations.

  - <tt>exp ^ exp</tt> -- Exponentiation.  This function uses the \c pow(3)
    function internally, but cannot be overridden, as can the parser
    builtin "pow" function.

  - <tt>exp \% exp</tt> -- The modulo function.  This function uses the \c
    fmod(3) function internally, but cannot be overridden, as can the parser
    builtin "fmod" function.

  - <tt>exp == exp, exp != exp, exp < exp, exp > exp, exp <= exp, exp >=
    exp</tt> -- Various tests.  The return value is 1 if the expression is
    true, 0 if it is false.  If both expressions are strings, a string
    comparison is done.  Equivalent operators: \c eq, \c ne, \c lt, \c gt,
    \c le, \c ge.

  - <tt>exp && exp, exp || exp</tt> -- Boolean AND and OR.  The return
    value is as above.  Equivalent operators: \c and, \c or.

  - <tt>exp =~ exp, exp !~ exp</tt> -- Pattern matching.  Return value is 1
    if the string matches (or doesn't match) the regexp string.  See \ref
    regex_syntax.  Equivalent operators: \c mat, \c nmat.

  - <tt>exp ? exp1 : exp2</tt> -- The C ternary operator.  If \c exp is
    nonzero the return value is \c exp1, otherwise \c exp2.

  - <tt>exp ; exp</tt> -- Concatenation of expressions.  This allows more
    than one expression per line.  The return value is the rightmost
    expression.

  - <tt>(exp)</tt> -- Parentheses.

  - <tt>+ exp, - exp</tt> -- Unary plus and minus.

  - <tt>variable</tt> -- The name of a variable.  The return value is the
    value of the variable.  If the variable is undefined, either 0 is
    returned or an error is raised (see vp_error_undef()).

  - <tt>variable = exp</tt> -- Variable assignment.  The return value is
    the value of the expression.

  - <tt>func([exp [, exp...]])</tt> -- Function call, where "func" is the
    name of a parser function.
  .
*/

/*!
  @defgroup parser_builtin Builtin functions and constants
  @ingroup parser

  The following functions are builtin, and are described in \c math(3).

  - Trigonometric functions -- <tt>sin, cos, tan, asin, acos, atan, atan2</tt>

  - Exponent and log functions -- <tt>exp, log, log10, pow, sqrt, hypot</tt>

  - Rounding functions -- <tt>ceil, floor, trunc, abs, fmod</tt>
  .

  The following functions are also defined:

  - <tt>min(val1, val2 [, val3...])</tt> -- Return the minimum of a list of
    values.

  - <tt>max(val1, val2 [, val3...])</tt> -- Return the maximum of a list of
    values.

  - <tt>randint([arg1 [, arg2]])</tt> -- Return random integer twixt two
    values.  If no arguments specified, between 0 and 1.  If one argument
    specified, between 0 and that value.  If two arguments specified, between
    those values.

  - <tt>randreal([arg1 [, arg2]])</tt> -- Like <tt>randint()</tt>, except
    for real values.
  .

  The following constants are predefined:

  - \c pi -- 3.14159265...

  - \c e -- 2.7182818...

  - \c dtor -- Degrees to radians conversion factor.

  - \c rtod -- Radians to degrees conversion factor.
  .
*/

#define CALL_MATH_1(func) \
        vs_dcreate(func(vl_dget(args, 0)))

#define CALL_MATH_2(func) \
        vs_dcreate(func(vl_dget(args, 0), vl_dget(args, 1)))

#define BUILTIN(func, macro) \
static vscalar *builtin_ ## func(vlist *args) { return macro(func); }

#define BUILTIN_1(func) \
        BUILTIN(func, CALL_MATH_1)

#define BUILTIN_2(func) \
        BUILTIN(func, CALL_MATH_2)

#define __NO_MATH_INLINES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "vars-config.h"
#include "vars-buffer.h"
#include "vars-hash.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-parser.h"
#include "vars-parser-lex.h"
#include "vars-random.h"

/* Type definition */
struct v_parser {
    struct v_header id;         /* Type marker */
    vhash *constants;           /* Constants */
    vhash *variables;           /* Variables */
    vhash *functions;           /* Functions */
    vscalar *value;             /* Last parse value */
    vlist *errors;              /* List of parse errors */
    int undef_ok;               /* OK to refer to undefined variables? */
};

/* Function attributes */
struct v_pfunc {
    vscalar *(*func)(vlist *args);
    int minargs, maxargs;
};

/* Internal type abbreviations */
typedef struct v_pfunc vpfunc;

/* Type variable */
vtype *vparser_type = NULL;

/* Whether to report line numbers */
static int report_line = 0;

/* Builtin math functions */
BUILTIN_1(sin); BUILTIN_1(cos); BUILTIN_1(tan);
BUILTIN_1(asin); BUILTIN_1(acos); BUILTIN_1(atan); BUILTIN_2(atan2);
BUILTIN_2(pow); BUILTIN_1(exp); BUILTIN_1(log); BUILTIN_1(log10);
BUILTIN_1(sqrt); BUILTIN_2(hypot);
BUILTIN_1(ceil); BUILTIN_1(floor);
BUILTIN_1(fabs); BUILTIN_2(fmod);

/* Other internal functions */
static void vp_add_builtins(vparser *p);
static vparser *vp_create_empty(void);
static vscalar *builtin_max(vlist *args);
static vscalar *builtin_min(vlist *args);
static vscalar *builtin_randint(vlist *args);
static vscalar *builtin_randreal(vlist *args);

/* List of builtin functions */
static struct builtin_func {
    char *name;
    vscalar *(*func)(vlist *args);
    int minargs, maxargs;
} builtin_functions[] = {
    { "sin",        builtin_sin,        1, 1               },
    { "cos",        builtin_cos,        1, 1               },
    { "tan",        builtin_tan,        1, 1               },

    { "asin",       builtin_asin,       1, 1               },
    { "acos",       builtin_acos,       1, 1               },
    { "atan",       builtin_atan,       1, 1               },
    { "atan2",      builtin_atan2,      2, 2               },

    { "pow",        builtin_pow,        2, 2               },
    { "exp",        builtin_exp,        1, 1               },
    { "log",        builtin_log,        1, 1               },
    { "log10",      builtin_log10,      1, 1               },

    { "sqrt",       builtin_sqrt,       1, 1               },
    { "hypot",      builtin_hypot,      2, 2               },

    { "ceil",       builtin_ceil,       1, 1               },
    { "floor",      builtin_floor,      1, 1               },

    { "abs",        builtin_fabs,       1, 1               },
    { "fmod",       builtin_fmod,       2, 2               },

    { "min",        builtin_min,        2, VP_FUNC_NOLIMIT },
    { "max",        builtin_max,        2, VP_FUNC_NOLIMIT },

    { "randint",    builtin_randint,    0, 2               },
    { "randreal",   builtin_randreal,   0, 2               },

    /* Terminator */
    { NULL, NULL, 0, 0 }
};

/* List of builtin constants */
static struct builtin_const {
    char *name;
    double value;
} builtin_constants[] = {
    { "pi",    M_PI       },    /* Pi */
    { "e",     M_E        },    /* E */
    { "dtor",  M_PI / 180 },    /* Degrees-to-radians */
    { "rtod",  180 / M_PI },    /* Radians-to-degrees */
    { NULL,    0.0        }
};

/* Add builtins to a parser */
static void
vp_add_builtins(vparser *p)
{
    int i;

    /* Install builtin constants */
    for (i = 0; builtin_constants[i].name != NULL; i++)
        vp_dconst(p,
                  builtin_constants[i].name,
                  builtin_constants[i].value);

    /* Install builtin functions */
    for (i = 0; builtin_functions[i].name != NULL; i++)
        vp_func(p,
                builtin_functions[i].name,
                builtin_functions[i].func,
                builtin_functions[i].minargs,
                builtin_functions[i].maxargs);
}

/*!
  @brief   Call a parser function.
  @ingroup parser_access
  @param   p Parser.
  @param   name Function name.
  @param   args List of arguments (or \c NULL, meaning no args).
  @return  Result.
  @retval  NULL if the call failed.
*/
vscalar *
vp_call(vparser *p, char *name, vlist *args)
{
    vscalar *result;
    int nargs = 0;
    vpfunc *f;

    VP_CHECK(p);
    if (args != NULL) {
        VL_CHECK(args);
        nargs = vl_length(args);
    }

    /* Get function info */
    if ((f = vh_pget(p->functions, name)) == NULL) {
        vp_err("function \"%s\" is undefined", name);
        return NULL;
    }

    /* Check no. of arguments */
    if (nargs < f->minargs || nargs > f->maxargs) {
        if (f->minargs == f->maxargs)
            vp_err("function \"%s\" requires %d argument%s",
                   name, f->minargs, (f->minargs == 1 ? "" : "s"));
        else if (f->maxargs == VP_FUNC_NOLIMIT)
            vp_err("function \"%s\" requires at least %d argument%s",
                   name, f->minargs, (f->minargs == 1 ? "" : "s"));
        else if (f->minargs == 0)
            vp_err("function \"%s\" requires at most %d argument%s",
                   name, f->maxargs, (f->maxargs == 1 ? "" : "s"));
        else
            vp_err("function \"%s\" requires between %d and %d arguments",
                   name, f->minargs, f->maxargs);

        return NULL;
    }

    /* Call function, checking for errors */
    errno = 0;
    result = f->func(args);
    if (errno && vp_errno(name, errno)) {
        if (result != NULL)
            vs_destroy(result);
        return NULL;
    }

    return result;
}

/* Check if pointer is a parser */
int
vp_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vparser_type);
}

/*!
  @brief   Declare a parser constant.
  @ingroup parser_custom
  @param   p Parser.
  @param   name Constant name.
  @param   val Its value.
*/
void
vp_const(vparser *p, char *name, vscalar *val)
{
    VP_CHECK(p);
    vh_istore(p->constants, name, 1);
    vh_store(p->variables, name, val);
}

/*!
  @brief   Return a copy of a parser.
  @ingroup parser_create
  @param   p Parser to copy.
  @return  Copy.

  Builtin functions and constants, and also current values of variables,
  are copied.
*/
vparser *
vp_copy(vparser *p)
{
    vpfunc *cf, *f;
    vparser *copy;
    viter iter;
    char *name;

    VP_CHECK(p);

    copy = vp_create_empty();

    vh_destroy(copy->constants);
    copy->constants = vh_copy(p->constants);

    vh_destroy(copy->variables);
    copy->variables = vh_copy(p->variables);

    v_iterate(p->functions, iter) {
        name = vh_iter_key(iter);
        f = vh_iter_pval(iter);
        cf = V_ALLOC(vpfunc, 1);
        cf->func = f->func;
        cf->minargs = f->minargs;
        cf->maxargs = f->maxargs;
        vh_pstore(copy->functions, name, cf);
    }

    return copy;
}

/*!
  @brief   Return a newly-created parser with builtins.
  @ingroup parser_create
  @return  New parser.
*/
vparser *
vp_create(void)
{
    vparser *p;

    p = vp_create_empty();
    vp_add_builtins(p);

    return p;
}

/* Return a newly-created empty parser */
static vparser *
vp_create_empty(void)
{
    static vheader *id = NULL;
    vparser *p;

    if (id == NULL) {
        vp_declare();
        id = v_header(vparser_type);
    }

    p = V_ALLOC(vparser, 1);
    p->id = *id;

    p->constants = vh_create();
    p->variables = vh_create();
    p->functions = vh_create();

    p->value = NULL;
    p->errors = NULL;
    p->undef_ok = 0;

    return p;
}

/* Declare parser type */
vtype *
vp_declare(void)
{
    if (vparser_type == NULL) {
        vparser_type = v_create("PARSER", "P");
	v_create_func(vparser_type, (void *(*)()) vp_create);
        v_copy_func(vparser_type, (void *(*)()) vp_copy);
        v_read_func(vparser_type, (void *(*)()) vp_read);
        v_write_func(vparser_type, vp_write);
        v_print_func(vparser_type, vp_print);
        v_destroy_func(vparser_type, vp_destroy);
    }

    return vparser_type;
}

/*!
  @brief   Delete a builtin constant.
  @ingroup parser_custom
  @param   p Parser.
  @param   name Constant to delete.

  Does nothing if the constant doesn't exist.
*/
void
vp_delconst(vparser *p, char *name)
{
    VP_CHECK(p);
    vh_delete(p->constants, name);
    vh_delete(p->variables, name);
}

/*!
  @brief   Delete a builtin function.
  @ingroup parser_custom
  @param   p Parser.
  @param   name Function to delete.

  Does nothing if the function doesn't exist.
*/
void
vp_delfunc(vparser *p, char *name)
{
    vpfunc *f;

    VP_CHECK(p);

    if ((f = vh_pget(p->functions, name)) != NULL) {
        vh_delete(p->functions, name);
        V_DEALLOC(f);
    }
}

/*!
  @brief   Deallocate a parser.
  @ingroup parser_create
  @param   p Parser.
*/
void
vp_destroy(vparser *p)
{
    char *name;
    viter iter;
    vpfunc *f;

    VP_CHECK(p);

    /* Destroy builtin functions */
    v_iterate(p->functions, iter) {
        name = vh_iter_key(iter);
        f = vh_iter_pval(iter);
        V_DEALLOC(f);
    }

    /* Destroy parser */
    vh_destroy(p->constants);
    vh_destroy(p->variables);
    vh_destroy(p->functions);

    if (p->value != NULL)
        vs_destroy(p->value);

    if (p->errors != NULL)
        vl_destroy(p->errors);

    V_DEALLOC(p);
}

/*!
  @brief   Flag a parse error.
  @ingroup parser_custom
  @param   fmt Format string.

  This function is for use inside a user-defined parser function to
  indicate that an error has occurred.
*/
void
vp_err(char *fmt, ...)
{
    extern vparser *vp_parser;
    V_NBUF_DECL(err);
    V_BUF_DECL;
    char *msg;

    if (vp_parser->errors == NULL)
        vp_parser->errors = vl_create();

    V_NBUF_FMT(err, fmt, msg);

    if (report_line)
        V_BUF_SET2("line %d: %s", scanner_line + 1, V_NBUF_VAL(err));
    else
        V_BUF_SET(V_NBUF_VAL(err));

    vl_spush(vp_parser->errors, V_BUF_VAL);
    v_exception("%s", V_BUF_VAL);
}

/* Flag an errno error */
int
vp_errno(char *name, int num)
{
    vp_err("function \"%s\": %s", name, strerror(num));
    return 1;
}

/*!
  @brief   Return the last parser error.
  @ingroup parser_exp
  @param   p Parser.
  @return  Error string.
  @retval  NULL if no errors occurred.
*/
char *
vp_error(vparser *p)
{
    VP_CHECK(p);
    return (p->errors != NULL ? vl_shead(p->errors) : NULL);
}

/*!
  @brief   Set whether undefined variable references cause an error.
  @ingroup parser_custom
  @param   p Parser.
  @param   flag Yes (the default) or no.
*/
void
vp_error_undef(vparser *p, int flag)
{
    VP_CHECK(p);
    p->undef_ok = !flag;
}

/*!
  @brief   Return the list of parser errors.
  @ingroup parser_exp
  @param   p Parser.
  @return  List of error strings.
  @retval  NULL if no errors occurred.

  If an evaluation failed, then one or more errors occurred.  This
  function returns the list of errors encountered during the last
  evaluation.
*/
vlist *
vp_errors(vparser *p)
{
    VP_CHECK(p);
    return p->errors;
}

/*!
  @brief   Evaluate an expression.
  @ingroup parser_exp
  @param   p Parser.
  @param   expr Expression.
  @return  Whether successful.

  Use vp_value() to get the value of the expression.
*/
int
vp_eval(vparser *p, char *expr)
{
    extern vscalar *vp_parse(vparser *p, char *expr);

    VP_CHECK(p);

    /* Initialise */
    report_line = (strchr(expr, '\n') != NULL);

    if (p->value != NULL)
        vs_destroy(p->value);

    if (p->errors != NULL)
        vl_destroy(p->errors);

    p->errors = NULL;

    /* Parse input */
    p->value = vp_parse(p, expr);

    /* Clean up */
    report_line = 0;

    return (p->errors == NULL);
}

/*!
  @brief   Return whether a variable exists.
  @ingroup parser_access
  @param   p Parser.
  @param   name Variable name.
  @return  Yes or no.
*/
int
vp_exists(vparser *p, char *name)
{
    VP_CHECK(p);
    return vh_exists(p->variables, name);
}

/*!
  @brief   Declare a parser function.
  @ingroup parser_custom
  @param   p Parser.
  @param   name Function name.
  @param   func Function.
  @param   minargs Minimum no. of arguments.
  @param   maxargs Maximum no. of arguments.
  @see     vp_func_min(), vp_func_max(), vp_func_args(), vp_func_any()

  Declare a parser function with the given name, function and range of
  arguments.  Overrides any previous definition.

  A parser function is one that accepts a list of scalars as arguments and
  returns a single scalar, which should be of type V_INT, V_DOUBLE or
  V_STRING.  In vp_eval(), the number of arguments supplied to the function
  must be between \c minargs and \c maxargs or a parse error is given.  If
  \c maxargs is VP_FUNC_NOLIMIT, then there is no upper limit to the number
  of arguments.  If the function fails for any reason, it should call
  vp_err() and return NULL.  The function should check for the case where
  \c args is \c NULL, and treat it as if an empty list of arguments was
  passed.
*/
void
vp_func(vparser *p, char *name, vscalar *(*func)(vlist *args),
        int minargs, int maxargs)
{
    vpfunc *f, *oldf;

    VP_CHECK(p);

    f = V_ALLOC(vpfunc, 1);
    f->func = func;
    f->minargs = V_MIN(minargs, maxargs);
    f->maxargs = V_MAX(minargs, maxargs);
    
    if ((oldf = vh_pget(p->functions, name)) != NULL)
        V_DEALLOC(oldf);

    vh_pstore(p->functions, name, f);
}

/*!
  @brief   Return the value of a parser variable.
  @ingroup parser_access
  @param   p Parser.
  @param   name Variable name.
  @return  Its value.
*/
vscalar *
vp_get(vparser *p, char *name)
{
    extern vparser *vp_parser;
    vscalar *val;

    VP_CHECK(p);

    if ((val = vh_get(p->variables, name)) == NULL &&
        !p->undef_ok && vp_parser != NULL)
        vp_err("undefined variable \"%s\"", name);

    return val;
}

/*!
  @brief   Return whether a name exists as a constant.
  @ingroup parser_access
  @param   p Parser.
  @param   name Name.
  @return  Yes or no.
*/
int
vp_isconst(vparser *p, char *name)
{
    VP_CHECK(p);
    return vh_exists(p->constants, name);
}


/*!
  @brief   Return whether a name exists as a function.
  @ingroup parser_access
  @param   p Parser.
  @param   name Name.
  @return  Yes or no.
*/
int
vp_isfunc(vparser *p, char *name)
{
    VP_CHECK(p);
    return vh_exists(p->functions, name);
}

/* Print contents of a parser */
void
vp_print(vparser *p, FILE *fp)
{
    vlist *list;
    char *name;
    viter iter;
    vpfunc *f;

    VP_CHECK(p);

    v_print_start();
    v_push_indent();

    v_print_type(vparser_type, p, fp);

    /* Print variables */
    v_indent(fp);
    fprintf(fp, "VARIABLES => LIST\n");
    v_push_indent();    

    list = vh_keys(p->variables);
    v_iterate(list, iter) {
        name = vl_iter_svalref(iter);
        v_indent(fp);
        fprintf(fp, "%s = %s", name, vh_sget(p->variables, name));
        if (vh_exists(p->constants, name))
            fprintf(fp, " (constant)");
        fprintf(fp, "\n");
    }

    v_pop_indent();

    /* Print functions */
    v_indent(fp);
    fprintf(fp, "FUNCTIONS => LIST\n");
    v_push_indent();    

    list = vh_keys(p->functions);
    v_iterate(list, iter) {
        name = vl_iter_svalref(iter);
        f = vh_pget(p->functions, name);
        v_indent(fp);
        fprintf(fp, "%s", name);

        if (f->minargs == f->maxargs)
            fprintf(fp, " (%d arg%s)", f->minargs,
                    (f->minargs == 1 ? "" : "s"));
        else if (f->maxargs == VP_FUNC_NOLIMIT)
            fprintf(fp, " (>= %d arg%s)", f->minargs,
                    (f->minargs == 1 ? "" : "s"));
        else if (f->minargs == 0)
            fprintf(fp, " (<= %d arg%s)", f->maxargs,
                    (f->maxargs == 1 ? "" : "s"));
        else
            fprintf(fp, " (%d-%d args)", f->minargs, f->maxargs);

        fprintf(fp, "\n");
    }

    v_pop_indent();

    /* That's it */
    v_pop_indent();
    v_print_finish();
}

/* Read parser from a stream */
vparser *
vp_read(FILE *fp)
{
    vparser *p;

    /* Initialise */
    p = vp_create_empty();
    vp_add_builtins(p);

    /* Read variables */
    vh_destroy(p->variables);
    if ((p->variables = vh_read(fp)) == NULL)
        return NULL;

    /* Read constant flags */
    vh_destroy(p->constants);
    if ((p->constants = vh_read(fp)) == NULL)
        return NULL;

    return p;
}

/*!
  @brief   Set the value of a parser variable.
  @ingroup parser_access
  @param   p Parser.
  @param   name Variable name.
  @param   val Value to set.
*/
void
vp_store(vparser *p, char *name, vscalar *val)
{
    VP_CHECK(p);

    if (!vh_exists(p->constants, name))
        vh_store(p->variables, name, val);
    else
        vp_err("attempt to modify constant \"%s\"", name);
}

/*!
  @brief   Return the value of the last expression evaluated.
  @ingroup parser_exp
  @param   p Parser.
  @return  Expression value.
*/
vscalar *
vp_value(vparser *p)
{
    VP_CHECK(p);
    return p->value;
}

/* Write parser to a stream */
int
vp_write(vparser *p, FILE *fp)
{
    VP_CHECK(p);

    /* Write variables */
    if (!vh_write(p->variables, fp))
        return 0;

    /* Write constant flags */
    if (!vh_write(p->constants, fp))
        return 0;

    return 1;
}

/* Builtin functions */
static vscalar *
builtin_max(vlist *args)
{
    double max = vl_dget(args, 0);
    viter iter;

    v_iterate(args, iter)
        max = V_MAX(max, vl_iter_dval(iter));

    return vs_dcreate(max);
}

static vscalar *
builtin_min(vlist *args)
{
    double min = vl_dget(args, 0);
    viter iter;

    v_iterate(args, iter)
        min = V_MIN(min, vl_iter_dval(iter));

    return vs_dcreate(min);
}

static vscalar *
builtin_randint(vlist *args)
{
    int min = 0, max;

    switch (vl_length(args)) {
    case 0:
        max = 1;
        break;
    case 1:
        max = vl_iget(args, 0);
        break;
    default:
        min = vl_iget(args, 0);
        max = vl_iget(args, 1);
        break;
    }

    return vs_icreate(v_randint(min, max));
}

static vscalar *
builtin_randreal(vlist *args)
{
    double min = 0.0, max;

    switch (vl_length(args)) {
    case 0:
        max = 1.0;
        break;
    case 1:
        max = vl_dget(args, 0);
        break;
    default:
        min = vl_dget(args, 0);
        max = vl_dget(args, 1);
        break;
    }

    return vs_dcreate(v_randreal(min, max));
}
