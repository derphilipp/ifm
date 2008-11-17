/*!
  @file
  @brief Array functions and macros.
  @ingroup array
*/

#ifndef VARS_ARRAY_H
#define VARS_ARRAY_H

#include <limits.h>
#include <vars-func.h>
#include <vars-list.h>

/*! @brief Abort if a pointer is not an array. */
#define VA_CHECK(ptr)           V_CHECK(ptr, va_check, "ARRAY")

/*! @brief Get array string value (pointer to internal buffer). */
#define va_sget(a, x)		vs_sget(va_get(a, x))

/*! @brief Get array string value by allocating a copy. */
#define va_sgetcopy(a, x)       vs_sgetcopy(va_get(a, x))

/*! @brief Get array string value by reference. */
#define va_sgetref(a, x)	vs_sgetref(va_get(a, x))

/*! @brief Get exact array string value (pointer to internal buffer). */
#define va_sget_exact(a, x)	vs_sget(va_get_exact(a, x))

/*! @brief Get exact array string value by allocating a copy. */
#define va_sgetcopy_exact(a, x) vs_sgetcopy(va_get_exact(a, x))

/*! @brief Get exact array string value by reference. */
#define va_sgetref_exact(a, x)	vs_sgetref(va_get_exact(a, x))

/*! @brief Set array string value. */
#define va_sstore(a, x, n)	va_store(a, x, vs_screate(n))

/*! @brief Get array float value. */
#define va_fget(a, x)		vs_fget(va_get(a, x))

/*! @brief Get exact array float value. */
#define va_fget_exact(a, x)	vs_fget(va_get_exact(a, x))

/*! @brief Set array float value. */
#define va_fstore(a, x, n)	va_store(a, x, vs_fcreate(n))

/*! @brief Get array double value. */
#define va_dget(a, x)		vs_dget(va_get(a, x))

/*! @brief Get exact array double value. */
#define va_dget_exact(a, x)	vs_dget(va_get_exact(a, x))

/*! @brief Set array double value. */
#define va_dstore(a, x, n)	va_store(a, x, vs_dcreate(n))

/*! @brief Get array integer value. */
#define va_iget(a, x)           vs_iget(va_get(a, x))

/*! @brief Get exact array integer value. */
#define va_iget_exact(a, x)     vs_iget(va_get_exact(a, x))

/*! @brief Set array integer value. */
#define va_istore(a, x, n)      va_store(a, x, vs_icreate(n))

/*! @brief Get array pointer value. */
#define va_pget(a, x)		vs_pget(va_get(a, x))

/*! @brief Get exact array pointer value. */
#define va_pget_exact(a, x)	vs_pget(va_get_exact(a, x))

/*! @brief Set array pointer value. */
#define va_pstore(a, x, p)	va_store(a, x, vs_pcreate((void *) p))

/*! @brief Iteration X value. */
#define va_iter_x(iter)         iter.fval[0]

/*! @brief Iteration scalar value. */
#define va_iter_val(iter)       v_iter_val(iter)

/*! @brief Iteration scalar integer value. */
#define va_iter_ival(iter)      v_iter_ival(iter)

/*! @brief Iteration scalar float value. */
#define va_iter_fval(iter)      v_iter_fval(iter)

/*! @brief Iteration scalar double value. */
#define va_iter_dval(iter)      v_iter_dval(iter)

/*! @brief Iteration scalar string value. */
#define va_iter_sval(iter)      v_iter_sval(iter)

/*! @brief Iteration scalar string value reference. */
#define va_iter_svalref(iter)   v_iter_svalref(iter)

/*! @brief Iteration scalar string value copy. */
#define va_iter_svalcopy(iter)  v_iter_svalcopy(iter)

/*! @brief Iteration scalar pointer value. */
#define va_iter_pval(iter)      v_iter_pval(iter)

/*! Terminator value for va_array() */
#define VA_END                  ((double) -LONG_MAX)

/* Fix for va_copy() macros */
#ifdef va_copy
#undef va_copy
#endif

/*! @brief Array type. */
typedef struct v_array varray;

#ifdef __cplusplus
extern "C" {
#endif

extern void va_add_list(varray *a, vlist *x, vlist *s);
extern varray *va_array(double x, ...);
extern int va_check(void *ptr);
extern varray *va_copy(varray *a);
extern varray *va_create(void);
extern varray *va_create_size(unsigned int size);
extern vtype *va_declare(void);
extern int va_defined(varray *a, float x);
extern int va_delete(varray *a, int num);
extern void va_destroy(varray *a);
extern void va_empty(varray *a);
extern int va_extrapolate(varray *a, float x, vscalar **before,
                          vscalar **after, float *fraction);
extern int va_find(varray *a, float x);
extern int va_find_nearest(varray *a, float x);
extern int va_freeze(varray *a, FILE *fp);
extern vscalar *va_get(varray *a, float x);
extern vscalar *va_get_exact(varray *a, float x);
extern int va_get_point(varray *a, int n, float *x, vscalar **s);
extern int va_interp(varray *a, float x, vscalar **before, vscalar **after,
                     float *fraction);
extern int va_lookup(varray *a, float x, enum v_interptype method,
                     vscalar *points[4], float fractions[4]);
extern float va_maximum(varray *a);
extern float va_minimum(varray *a);
extern int va_next(viter *iter);
extern int va_point_count(varray *a);
extern vlist *va_points_s(varray *a);
extern vlist *va_points_x(varray *a);
extern void va_print(varray *a, FILE *fp);
extern varray *va_read(FILE *fp);
extern void va_remove_func(varray *a, void (*remove)(vscalar *s));
extern void va_resolution(varray *a, float res);
extern void va_store(varray *a, float x, vscalar *s);
extern varray *va_thaw(FILE *fp);
extern int va_traverse(varray *a, int (*func) (void *ptr));
extern int va_write(varray *a, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
