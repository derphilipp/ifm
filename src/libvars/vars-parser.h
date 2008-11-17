/*!
  @file
  @brief Parser functions and macros.
  @ingroup parser
*/

#ifndef VARS_PARSER_H
#define VARS_PARSER_H

#include <vars-list.h>

/*! @brief Abort if a pointer is not a parser. */
#define VP_CHECK(ptr)            V_CHECK(ptr, vp_check, "PARSER")

/*! @brief Declare an integer parser constant. */
#define vp_iconst(p, name, val)  vp_const(p, name, vs_icreate(val))

/*! @brief Declare a double parser constant. */
#define vp_dconst(p, name, val)  vp_const(p, name, vs_dcreate(val))

/*! @brief Declare a string parser constant. */
#define vp_sconst(p, name, val)  vp_const(p, name, vs_screate(val))

/*! @brief Set an integer parser variable. */
#define vp_istore(p, name, val)  vp_store(p, name, vs_icreate(val))

/*! @brief Set a double parser variable. */
#define vp_dstore(p, name, val)  vp_store(p, name, vs_dcreate(val))

/*! @brief Set a string parser variable. */
#define vp_sstore(p, name, val)  vp_store(p, name, vs_screate(val))

/*! @brief Get the value of an integer parser variable. */
#define vp_iget(p, name)         vs_iget(vp_get(p, name))

/*! @brief Get the value of a double parser variable. */
#define vp_dget(p, name)         vs_dget(vp_get(p, name))

/*! @brief Get the value of a string parser variable. */
#define vp_sget(p, name)         vs_sget(vp_get(p, name))

/*! @brief Get a reference to the value of a string parser variable. */
#define vp_sgetref(p, name)      vs_sgetref(vp_get(p, name))

/*! @brief Get a copy of the value of a string parser variable. */
#define vp_sgetcopy(p, name)     vs_sgetcopy(vp_get(p, name))

/*! @brief Get the integer value of the last expression. */
#define vp_ivalue(p)             vs_iget(vp_value(p))

/*! @brief Get the double value of the last expression. */
#define vp_dvalue(p)             vs_dget(vp_value(p))

/*! @brief Get the string value of the last expression. */
#define vp_svalue(p)             vs_sget(vp_value(p))

/*! @brief Declare a parser function with a minimum no. of arguments. */
#define vp_func_min(p, name, func, min) \
        vp_func(p, name, func, min, VP_FUNC_NOLIMIT)

/*! @brief Declare a parser function with a maximum no. of arguments. */
#define vp_func_max(p, name, func, max) \
        vp_func(p, name, func, 0, max)

/*! @brief Declare a parser function with a specific no. of arguments. */
#define vp_func_args(p, name, func, num) \
        vp_func(p, name, func, num, num)

/*! @brief Declare a parser function with any number of arguments. */
#define vp_func_any(p, name, func) \
        vp_func(p, name, func, 0, VP_FUNC_NOLIMIT)

#define VP_FUNC_NOLIMIT 1000000

/*! @brief Parser type. */
typedef struct v_parser vparser;

#ifdef __cplusplus
extern "C" {
#endif

extern vscalar *vp_call(vparser *p, char *name, vlist *args);
extern int vp_check(void *ptr);
extern void vp_const(vparser *p, char *name, vscalar *val);
extern vparser *vp_copy(vparser *p);
extern vparser *vp_create(void);
extern vtype *vp_declare(void);
extern void vp_delconst(vparser *p, char *name);
extern void vp_delfunc(vparser *p, char *name);
extern void vp_destroy(vparser *p);
extern void vp_err(char *fmt, ...);
extern int vp_errno(char *name, int num);
extern char *vp_error(vparser *p);
extern void vp_error_undef(vparser *p, int flag);
extern vlist *vp_errors(vparser *p);
extern int vp_eval(vparser *p, char *expr);
extern int vp_exists(vparser *p, char *name);
extern void vp_func(vparser *p, char *name, vscalar *(*func)(vlist *args),
                    int minargs, int maxargs);
extern vscalar *vp_get(vparser *p, char *name);
extern int vp_isconst(vparser *p, char *name);
extern int vp_isfunc(vparser *p, char *name);
extern void vp_print(vparser *p, FILE *fp);
extern vparser *vp_read(FILE *fp);
extern void vp_store(vparser *p, char *name, vscalar *val);
extern vscalar *vp_value(vparser *p);
extern int vp_write(vparser *p, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
