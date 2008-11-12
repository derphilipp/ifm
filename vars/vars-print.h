/*!
  @file
  @brief Printing functions.
*/

#ifndef VARS_PRINT_H
#define VARS_PRINT_H

#include <vars-type.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void v_indent(FILE *fp);
extern void v_indent_width(int num);
extern void v_pop_indent(void);
extern void v_print(void *ptr, FILE *fp);
extern void v_print_address(int flag);
extern void v_print_default(void (*func)(void *ptr, FILE *fp));
extern void v_print_finish(void);
extern void v_print_start(void);
extern void v_print_type(vtype *type, void *ptr, FILE *fp);
extern void v_print_with(void *ptr, void (*func)(void *ptr, FILE *fp));
extern void v_push_indent(void);
extern void v_show(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
