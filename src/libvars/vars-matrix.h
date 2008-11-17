/*!
  @file
  @brief Matrix functions and macros.
  @ingroup matrix
*/

#ifndef VARS_MATRIX_H
#define VARS_MATRIX_H

#include <vars-type.h>

/*! @brief Abort if a pointer is not a matrix. */
#define VM_CHECK(ptr)      V_CHECK(ptr, vm_check, "MATRIX")

/*! @brief Matrix type. */
typedef struct v_matrix vmatrix;

#ifdef __cplusplus
extern "C" {
#endif

extern int vm_check(void *ptr);
extern int vm_columns(vmatrix *m);
extern vmatrix *vm_copy(vmatrix *m);
extern vmatrix *vm_create(void);
extern vmatrix *vm_create_size(int nrows, int ncols);
extern vtype *vm_declare(void);
extern void vm_destroy(vmatrix *m);
extern int vm_freeze(vmatrix *m, FILE *fp);
extern float vm_get(vmatrix *m, int row, int col);
extern void vm_print(vmatrix *m, FILE *fp);
extern vmatrix *vm_read(FILE *fp);
extern void vm_require(vmatrix *m, int nrows, int ncols);
extern int vm_rows(vmatrix *m);
extern void vm_set_size(vmatrix *m, int nrows, int ncols);
extern void vm_store(vmatrix *m, int row, int col, float val);
extern vmatrix *vm_thaw(FILE *fp);
extern int vm_traverse(vmatrix *m, int (*func)());
extern int vm_write(vmatrix *m, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
