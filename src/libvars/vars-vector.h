/*!
  @file
  @brief Vector functions and macros.
  @ingroup vector
*/

#ifndef VARS_VECTOR_H
#define VARS_VECTOR_H

#include <vars-list.h>

/*! @brief Abort of a pointer is not a vector. */
#define VV_CHECK(ptr)      V_CHECK(ptr, vv_check, "VECTOR")

/*! @brief Vector type. */
typedef struct v_vector vvector;

#ifdef __cplusplus
extern "C" {
#endif

extern int vv_check(void *ptr);
extern vvector *vv_copy(vvector *v);
extern vvector *vv_create(unsigned int size);
extern vvector *vv_create_from_list(vlist *list);
extern vvector *vv_create_from_values(unsigned int size, float *values);
extern vvector *vv_create_vector(unsigned int size, ...);
extern vvector *vv_create_with_values(unsigned int size, float *values);
extern vtype *vv_declare(void);
extern void vv_destroy(vvector *v);
extern int vv_freeze(vvector *v, FILE *fp);
extern void vv_freeze_format(char *string);
extern void vv_freeze_linesize(int size);
extern float vv_get(vvector *v, int num);
extern float *vv_get_values(vvector *v);
extern float *vv_get_values_copy(vvector *v);
extern int vv_length(vvector *v);
extern void vv_print(vvector *v, FILE *fp);
extern void vv_print_format(char *string);
extern void vv_print_linesize(int size);
extern vvector *vv_read(FILE *fp);
extern void vv_sort(vvector *v);
extern void vv_store(vvector *v, int num, float val);
extern void vv_store_values(vvector *v, float *values);
extern vvector *vv_thaw(FILE *fp);
extern int vv_traverse(vvector *v, int (*func)());
extern int vv_write(vvector *v, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
