/*!
  @file
  @brief Function functions and macros.
  @ingroup func
*/

#ifndef VARS_FUNC_H
#define VARS_FUNC_H

#include <limits.h>
#include <vars-list.h>

/*! @brief Abort if a pointer is not a function. */
#define VF_CHECK(ptr)            V_CHECK(ptr, vf_check, "FUNC")

/*! @brief Iteration X value. */
#define vf_iter_x(iter)         iter.fval[0]

/*! @brief Iteration FX value. */
#define vf_iter_fx(iter)        iter.fval[1]

/*! Terminator value for vf_func() */
#define VF_END                   ((double) -LONG_MAX)

/*! @brief Function interpolation types. */
enum v_interptype {
    V_INTERP_LINEAR,            /*!< Linear */
    V_INTERP_LAGRANGE,          /*!< Lagrange */
    V_INTERP_SPLINE,            /*!< Spline */
    V_INTERP_NEAREST,           /*!< Nearest point */
    V_INTERP_EXTRAPOLATE        /*!< Linear with extrapolation  */
};

/*! @brief Function type. */
typedef struct v_func vfunc;

#ifdef __cplusplus
extern "C" {
#endif

extern void vf_add_array(vfunc *f, int num, double *x, double *fx);
extern void vf_add_func(vfunc *f, double (*func)(double val), double min,
                        double max, unsigned int num);
extern void vf_add_list(vfunc *f, vlist *x, vlist *fx);
extern void vf_add_point(vfunc *f, double x, double fx);
extern int vf_check(void *ptr);
extern vfunc *vf_copy(vfunc *f);
extern vfunc *vf_create(void);
extern vfunc *vf_create_size(unsigned int size);
extern vtype *vf_declare(void);
extern int vf_defined(vfunc *f, double x);
extern int vf_delete_point(vfunc *f, int num);
extern void vf_destroy(vfunc *f);
extern vfunc *vf_differential(vfunc *f);
extern void vf_empty(vfunc *f);
extern int vf_freeze(vfunc *f, FILE *fp);
extern vfunc *vf_func(double x, double fx, ...);
extern int vf_get_point(vfunc *f, int n, double *x, double *fx);
extern int vf_get_range(vfunc *f, double *fmin, double *fmax);
extern vfunc *vf_integral(vfunc *f);
extern void vf_interpolate(vfunc *f, enum v_interptype type);
extern double vf_maximum(vfunc *f);
extern double vf_minimum(vfunc *f);
extern int vf_next(viter *iter);
extern int vf_point_count(vfunc *f);
extern vlist *vf_points_fx(vfunc *f);
extern vlist *vf_points_x(vfunc *f);
extern void vf_print(vfunc *f, FILE *fp);
extern vfunc *vf_read(FILE *fp);
extern vfunc *vf_reduce(vfunc *f, double tolerance);
extern void vf_resolution(vfunc *f, double res);
extern void vf_set_cycle(vfunc *f, double cycle);
extern void vf_spline_deriv(vfunc *f, double first, double last);
extern vfunc *vf_thaw(FILE *fp);
extern vfunc *vf_transform(vfunc *f, double (*xfunc) (double x),
                           double (*fxfunc) (double fx));
extern int vf_traverse(vfunc *f, int (*func)(void *ptr));
extern double vf_value(vfunc *f, double x);
extern double vf_value_extrapolate(vfunc *f, double x);
extern double vf_value_lagrange(vfunc *f, double x);
extern double vf_value_linear(vfunc *f, double x);
extern double vf_value_nearest(vfunc *f, double x);
extern double vf_value_spline(vfunc *f, double x);
extern int vf_write(vfunc *f, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
