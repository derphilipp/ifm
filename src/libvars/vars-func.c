/*!
  @defgroup func Functions
  @ingroup types

  A function is an object which contains a set of pairs of real numbers (X,
  FX), the second of which is a function of the first (i.e. FX = F(X)).
  This is the mathematical definition of a (real-valued) function, and
  should not be confused with the sort of functions found in C.
*/

/*!
  @defgroup func_create Creating and destroying functions
  @ingroup func
*/

/*!
  @defgroup func_modify Modifying functions
  @ingroup func

  These functions are available for adding and removing points from a
  function.  Note that (X, FX) points can be added in any order -- when
  required, the points are sorted by increasing X value.

  When a point is added to a function, any existing point with the same X
  value is replaced by it.  The definition of 'same X value' is any point
  with an X value which differs by less than the resolution of the
  function.  The default resolution is 1.0e-8. The vf_resolution() function
  can change it.
*/

/*!
  @defgroup func_interp Interpolating function values
  @ingroup func

  Function interpolation is the process of finding the function values of
  points which are not explicitly defined in the function.  When doing
  interpolation, all functions are assumed to be continuous -- i.e. the
  value of a function at a given point can be estimated by some sort of
  interpolation of values at the surrounding points. There are several
  types of interpolation available. These are:

  - V_INTERP_LINEAR -- Simple linear interpolation between the closest two
    points.
  - V_INTERP_LAGRANGE -- Lagrange 4-point interpolation, based on the
    surrounding four points (as its name implies).  This gives fairly
    smooth interpolation. If the function doesn't have enough points,
    linear interpolation is done instead.
  - V_INTERP_SPLINE -- Cubic spline interpolation.  This gives smoother
    interpolation than Lagrange interpolation, but at a slight run-time
    cost. If the function doesn't have enough points, linear interpolation
    is done instead.
  - V_INTERP_NEAREST -- Not really interpolation at all -- just use the
    function value at the closest point.
  - V_INTERP_EXTRAPOLATE -- Same as linear interpolation, but extrapolates
    outside the function range.  All the other methods "flat line" when the
    look up value is outside the function range.
  .

  There are two ways of specifying the type of interpolation you want done
  in a function -- firstly, by setting the implicit type of interpolation in
  the function itself, and secondly, by using a particular type of
  interpolation explicitly.
*/

/*!
  @defgroup func_access Retrieving function information
  @ingroup func
*/

/*!
  @defgroup func_math Mathematical operations
  @ingroup func
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "vars-config.h"
#include "vars-func.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-yaml.h"

#ifndef HAVE_DREM
#define drem v_drem
#endif

#ifndef FUNC_DEFAULT_SIZE
#define FUNC_DEFAULT_SIZE 20
#endif

#define FLOAT_FORMAT "%.12g"

/* Various shortcut macros */
#define TYPE(f)     ((f)->interp)
#define NP(f)       ((f)->numpoints)
#define INITP(f)    ((f)->initpoints)
#define MAXP(f)     ((f)->maxpoints)
#define MOD(f)      ((f)->modflag)
#define RES(f)      ((f)->resolution)

#define XVAL(f, n)  ((f)->points[n].x)
#define FXVAL(f, n) ((f)->points[n].fx)
#define CODE(f, n)  ((f)->points[n].code)

#define XMIN(f)     XVAL(f, 0)
#define XMAX(f)     XVAL(f, NP(f) - 1)
#define EMPTY(f)    (NP(f) == 0)

#define X3X(x)      ((x) * (x) * (x) - (x))

#define SORT(f)     qsort((f)->points, NP(f), sizeof(vpoint), vf_sortpoints)

#define CYCLE(f, fx) do {                       \
    fx = drem(fx, (f)->cycle);                  \
    if (fx < 0.0)                               \
        fx += f->cycle;                         \
} while (0)

#define CYCLE_TEST(f, fx) \
        if ((f)->cycle > 0.0) CYCLE(f, fx)

#define CYCLE_DIFF(f, fxdiff, fx2, fx1)	do {    \
    fxdiff = fx2 - fx1;                         \
    if (f->cycle > 0.0)                         \
        fxdiff = drem(fxdiff, f->cycle);        \
} while (0)

/* Add a point to a function */
#define ADDP(f, x, fx) do {                                     \
    if (NP(f) == MAXP(f) - 1) {                                 \
        MAXP(f) += INITP(f);                                    \
        f->points = V_REALLOC(f->points, vpoint, MAXP(f));      \
    }                                                           \
                                                                \
    XVAL(f, NP(f)) = x;                                         \
    FXVAL(f, NP(f)) = fx;                                       \
    CODE(f, NP(f)) = ++sortcode;                                \
    MOD(f) = 1;                                                 \
    NP(f)++;                                                    \
} while (0)

/* Delete a point from a function */
#define DELP(f, num) do {						\
    size_t points_after = (size_t) (NP(f) - num - 1);			\
    if (points_after > 0)						\
	memmove((void*) &f->points[num],				\
		(void*) &f->points[num + 1],				\
		(size_t) points_after * sizeof(struct v_point));	\
    NP(f)--;								\
} while (0)

/* A single point */
/*@-redef@*/
struct v_point {
    double x;                   /* X value */
    double fx;                  /* Function value */
    int code;                   /* Sort code */
};

typedef struct v_point vpoint;
/*@=redef@*/

/* Spline parameters */
struct v_spline {
    double *d, *u, *w, *p;      /* See Sedgewick for what these are */
};

typedef struct v_spline vspline;

/* Type definition */
struct v_func {
    struct v_header id;         /* Type marker */

    struct v_point *points;     /* Function points */
    unsigned int initpoints;    /* Initial point allocation */
    unsigned int maxpoints;     /* Max. points allocated */
    unsigned int numpoints;     /* No. of points used */

    double resolution;          /* Resolution of X values */
    int modflag;                /* Function modified? */
    enum v_interptype interp;   /* Interpolation type */
    double cycle;               /* Interpolation cycle */

    struct v_spline *spline;    /* Spline data */
    double sd1, sd2;            /* First/last spline derivatives */
};

/* Type variable */
vtype *vfunc_type = NULL;

/* Sort code values */
static int sortcode = 0;

/* X value resolution */
static double resolution;

/* Duplicate-point flag */
static int duplicate;

/* Internal functions */
static vspline *vf_create_spline(vfunc *f);
static void vf_destroy_spline(vspline *s);
static int vf_search(vfunc *f, double x, int *entry, double *fraction);
static int vf_search_extrapolate(vfunc *f, double x, int *entry,
                                 double *fraction);
static int vf_sortpoints(const void *v1, const void *v2);
static void vf_update(vfunc *f, int spline);
static int vf_xmldump(vfunc *f, FILE *fp);
static int vf_yamldump(vfunc *f, FILE *fp);

/*!
  @brief   Add an array of points to a function.
  @ingroup func_modify
  @param   f Function.
  @param   num No. of points to add.
  @param   x List of X points.
  @param   fx List of FX points.
*/
void
vf_add_array(vfunc *f, int num, double *x, double *fx)
{
    int i;

    VF_CHECK(f);

    for (i = 0; i < num; i++)
        ADDP(f, x[i], fx[i]);
}

/*!
  @brief   Add points returned from a function.
  @ingroup func_modify
  @param   f Function.
  @param   func Function to get points from.
  @param   min Minimum value.
  @param   max Maximum value.
  @param   num No. of points to add.

  The supplied \c func is called \c num times, from \c min to \c max, and
  the returned FX points are added.
*/
void
vf_add_func(vfunc *f, double (*func)(double val),
            double min, double max, unsigned int num)
{
    double val, fraction;
    int i;

    VF_CHECK(f);

    if (num < 2) {
        v_exception("vf_add_func(): invalid number of points");
        return;
    }

    for (i = 0; i < num; i++) {
        fraction = ((double) i) / (num - 1);
        val = min + fraction * (max - min);
        ADDP(f, val, func(val));
    }
}

/*!
  @brief   Add a list of points to a function.
  @ingroup func_modify
  @param   f Function.
  @param   x List of X points.
  @param   fx List of FX points.
*/
void
vf_add_list(vfunc *f, vlist *x, vlist *fx)
{
    int i, num;

    VF_CHECK(f);

    VL_CHECK(x);
    VL_CHECK(fx);

    num = V_MAX(vl_length(x), vl_length(fx));

    for (i = 0; i < num; i++)
        ADDP(f, vl_dget(x, i), vl_dget(fx, i));
}

/*!
  @brief   Add a point to a function.
  @ingroup func_modify
  @param   f Function.
  @param   x X value.
  @param   fx FX value.
*/
void
vf_add_point(vfunc *f, double x, double fx)
{
    VF_CHECK(f);
    ADDP(f, x, fx);
}

/* Check if pointer is a function */
int
vf_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vfunc_type);
}

/*!
  @brief   Return a copy of a function.
  @ingroup func_create
  @param   f Function to copy.
  @return  The copy.
*/
vfunc *
vf_copy(vfunc *f)
{
    vfunc *copy;
    int i;

    VF_CHECK(f);

    copy = vf_create_size(INITP(f));

    for (i = 0; i < NP(f); i++)
        ADDP(copy, XVAL(f, i), FXVAL(f, i));

    RES(copy) = RES(f);
    TYPE(copy) = TYPE(f);

    return copy;
}

/*!
  @brief   Return a newly-created function.
  @ingroup func_create
  @return  New function.
*/
vfunc *
vf_create(void)
{
    return vf_create_size(FUNC_DEFAULT_SIZE);
}

/*!
  @brief   Return a new function with a given initial size.
  @ingroup func_create
  @param   size Initial size.
  @return  New function.
*/
vfunc *
vf_create_size(unsigned int size)
{
    static vheader *id = NULL;
    vfunc *f;

    if (id == NULL) {
        vf_declare();
        id = v_header(vfunc_type);
    }

    f = V_ALLOC(vfunc, 1);
    f->id = *id;

    f->points = V_ALLOC(vpoint, size);
    f->initpoints = V_MAX(size, 1);
    f->maxpoints = V_MAX(size, 1);
    f->numpoints = 0;
    f->modflag = 0;
    f->resolution = 1.0e-8;
    f->interp = V_INTERP_LINEAR;
    f->cycle = -1.0;

    f->spline = NULL;
    f->sd1 = f->sd2 = 0.0;

    return f;
}

/* Create and return new spline parameters */
static vspline *
vf_create_spline(vfunc *f)
{
    vspline *s;
    double sp, sm, fx;
    int i;

    s = V_ALLOC(vspline, 1);

    s->d = V_ALLOC(double, NP(f));
    s->u = V_ALLOC(double, NP(f));
    s->w = V_ALLOC(double, NP(f));
    s->p = V_ALLOC(double, NP(f));

    for (i = 0; i < NP(f); i++)
        s->d[i] = s->u[i] = s->w[i] = s->p[i] = 0.0;

    for (i = 1; i < NP(f) - 1; i++)
        s->d[i] = 2 * (XVAL(f, i + 1) - XVAL(f, i - 1));

    for (i = 0; i < NP(f) - 1; i++)
        s->u[i] = XVAL(f, i + 1) - XVAL(f, i);

    for (i = 1; i < NP(f) - 1; i++) {
	fx = FXVAL(f, i);
	sp = FXVAL(f, i + 1) - fx;
	sm = fx - FXVAL(f, i - 1);

	if (f->cycle > 0.0) {
	    sp = drem(sp, f->cycle);
	    sm = drem(sm, f->cycle);
	}

        s->w[i] = 6 * ( sp / s->u[i] - sm / s->u[i - 1]);
    }

    for (i = 1; i < NP(f) - 1; i++) {
        s->w[i + 1] -= s->w[i] * s->u[i] / s->d[i];
        s->d[i + 1] -= s->u[i] * s->u[i] / s->d[i];
    }

    s->p[0] = f->sd1;
    s->p[NP(f) - 1] = f->sd2;

    for (i = NP(f) - 2; i > 0; i--)
        s->p[i] = (s->w[i] - s->u[i] * s->p[i + 1]) / s->d[i];

    return s;
}

/* Declare function type */
vtype *
vf_declare(void)
{
    if (vfunc_type == NULL) {
        vfunc_type = v_create("FUNC", "F");
	v_create_func(vfunc_type, (void *(*)()) vf_create);
        v_copy_func(vfunc_type, (void *(*)()) vf_copy);
        v_next_func(vfunc_type, vf_next);
        v_read_func(vfunc_type, (void *(*)()) vf_read);
        v_write_func(vfunc_type, vf_write);
        v_freeze_func(vfunc_type, vf_freeze);
        v_thaw_func(vfunc_type, (void *(*)()) vf_thaw);
        v_print_func(vfunc_type, vf_print);
        v_xmldump_func(vfunc_type, vf_xmldump);
        v_yamldump_func(vfunc_type, vf_yamldump);
        v_destroy_func(vfunc_type, vf_destroy);
        v_traverse_func(vfunc_type, vf_traverse);
    }

    return vfunc_type;
}

/*!
  @brief   Return whether an interpolation X value is defined.
  @ingroup func_access
  @param   f Function.
  @param   x X value.
  @return  Yes or no.

  Return whether a function is defined at a given point (i.e. whether the
  point is between the minimum and maximum X values).  This is always false
  for an empty function.
*/
int
vf_defined(vfunc *f, double x)
{
    VF_CHECK(f);
    vf_update(f, 0);

    if (EMPTY(f))
        return 0;

    return (x >= XMIN(f) && x <= XMAX(f));
}

/*!
  @brief   Delete a function point.
  @ingroup func_modify
  @param   f Function.
  @param   num Point number to delete (counting from zero).
  @return  Whether anything was deleted.
*/
int
vf_delete_point(vfunc *f, int num)
{
    VF_CHECK(f);

    if (num < 0 || num >= NP(f))
        return 0;

    DELP(f, num);

    /* Any spline will have been invalidated */
    if (f->spline != NULL) {
        vf_destroy_spline(f->spline);
	f->spline = NULL;
    }

    return 1;
}

/*!
  @brief   Deallocate a function.
  @ingroup func_create
  @param   f Function.
*/
void
vf_destroy(vfunc *f)
{
    VF_CHECK(f);

    if (f->spline != NULL)
        vf_destroy_spline(f->spline);

    V_DEALLOC(f->points);
    V_DEALLOC(f);
}

/* Destroy spline parameters */
static void
vf_destroy_spline(vspline *s)
{
    V_DEALLOC(s->d);
    V_DEALLOC(s->u);
    V_DEALLOC(s->w);
    V_DEALLOC(s->p);
    V_DEALLOC(s);
}

/*!
  @brief   Differentiate a function.
  @ingroup func_math
  @param   f Function.
  @return  Differentiated function.
  @retval  NULL if function doesn't have enough points.
*/
vfunc *
vf_differential(vfunc *f)
{
    double x0, x1, x2, y0, y1, y2, s0, s1, ydiff;
    vfunc *diff;
    int i, n;

    VF_CHECK(f);

    /* Have we enough points? */
    if ((n = NP(f)) <= 1)
	return NULL;

    vf_update(f, 0);
    diff = vf_create_size(n);

    x0 = XVAL(f, 0);
    y0 = FXVAL(f, 0);
    x1 = XVAL(f, 1);
    y1 = FXVAL(f, 1);

    if (n <= 2) {
	/* Two points only, constant slope */
	CYCLE_DIFF(f, ydiff, y1, y0);
	s1 = ydiff / (x1 - x0);
	ADDP(diff, x0, s1);
	ADDP(diff, x1, s1);
    } else {
	for (i = 2; i < n; i++) {
	    x2 = XVAL(f, i);
	    y2 = FXVAL(f, i);
	    CYCLE_DIFF(f, ydiff, y2, y0);
	    s1 = ydiff / (x2 - x0);

	    if (i == 2) {
		/* Insert first slope */
		CYCLE_DIFF(f, ydiff, y1, y0);
		s0 = 2.0 * ydiff / (x1 - x0) - s1;
		ADDP(diff, x0, s0);
	    }

	    ADDP(diff, x1, s1);

	    if (i >= n - 1) {
		/* Add last point */
		CYCLE_DIFF(f, ydiff, y2, y1);
		s0 = 2.0 * ydiff / (x2 - x1) - s1;
		ADDP(diff, x2, s0);
		break;
	    }

	    /* Shuffle numbers along  */
	    x0 = x1;
	    x1 = x2;
	    y0 = y1;
	    y1 = y2;
	}
    }

    return diff;
}

/*!
  @brief   Empty a function of all points.
  @ingroup func_modify
  @param   f Function.
*/
void
vf_empty(vfunc *f)
{
    VF_CHECK(f);
    NP(f) = 0;
}

/* Freeze a function to a stream */
int
vf_freeze(vfunc *f, FILE *fp)
{
    int i;

    VF_CHECK(f);
    vf_update(f, 0);

    v_freeze_start(fp);

    switch (f->interp) {

    case V_INTERP_LAGRANGE:
        fputs("LAGRANGE ", fp);
        break;

    case V_INTERP_SPLINE:
        fputs("SPLINE ", fp);
        break;

    case V_INTERP_NEAREST:
        fputs("NEAREST ", fp);
        break;

    case V_INTERP_EXTRAPOLATE:
        fputs("EXTRAPOLATE ", fp);
        break;
    }

    fputs("{\n", fp);
    v_push_indent();

    for (i = 0; i < f->numpoints; i++) {
        v_indent(fp);
        fprintf(fp, FLOAT_FORMAT, f->points[i].x);
        fprintf(fp, "\t");
        fprintf(fp, FLOAT_FORMAT, f->points[i].fx);
        fputs(",\n", fp);
    }

    v_pop_indent();
    v_indent(fp);
    fputc('}', fp);

    v_freeze_finish(fp);

    return 1;
}

/*!
  @brief   Build a function from a list of (X, FX) values.
  @ingroup func_create
  @return  New function.

  The list of values must terminate with an X value of VF_END, or chaos
  will result.
*/
vfunc *
vf_func(double x, double fx, ...)
{
    va_list ap;
    vfunc *f;

    f = vf_create();
    ADDP(f, x, fx);
    va_start(ap, fx);

    while (1) {
        if ((x = va_arg(ap, double)) != VF_END) {
            fx = va_arg(ap, double);
            ADDP(f, x, fx);
        } else {
            break;
        }
    }

    va_end(ap);
    return f;
}

/*!
  @brief   Return an (X, FX) point in a function.
  @ingroup func_access
  @param   f Function.
  @param   n Point number.
  @param   x Pointer to returned X value.
  @param   fx Pointer to returned FX value.
  @return  Whether a point was returned.
*/
int
vf_get_point(vfunc *f, int n, double *x, double *fx)
{
    VF_CHECK(f);

    if (n < 0 || n >= NP(f))
        return 0;

    vf_update(f, 0);
    *x = XVAL(f, n);
    *fx = FXVAL(f, n);

    return 1;
}

/*!
  @brief   Return the min/max function values of a function.
  @ingroup func_access
  @param   f Function.
  @param   fmin Pointer to minimum FX value.
  @param   fmax Pointer to maximum FX value.
  @return  Whether anything was returned.
*/
int
vf_get_range(vfunc *f, double *fmin, double *fmax)
{
    int i;

    VF_CHECK(f);

    if (EMPTY(f))
        return 0;

    *fmin = *fmax = FXVAL(f, 0);
    for (i = 1; i < NP(f); i++) {
        *fmin = V_MIN(*fmin, FXVAL(f, i));
        *fmax = V_MAX(*fmax, FXVAL(f, i));
    }

    return 1;
}

/*!
  @brief   Integrate a function.
  @ingroup func_math
  @param   f Function.
  @return  Integrated function.
  @retval  NULL if function doesn't have enough points.
*/
vfunc *
vf_integral(vfunc *f)
{
    /* Integral formula based on cubic fit */
#   define INTEGRAL (0.5 * del_x * ((y0 + y1) + \
                    (1.0 / 6.0) * del_x * (s1 - s0)))

    double s0, s1, del_x;
    double x0, x1, x2;
    double y0, y1, y2;
    double sum = 0.0;
    vfunc *integral;
    int i, n;

    VF_CHECK(f);

    /* Have we enough points */
    if ((n = NP(f)) <= 1)
	return NULL;

    vf_update(f, 0);
    integral = vf_create_size(n);

    /* First point always zero */
    x0 = XVAL(f, 0);
    y0 = FXVAL(f, 0);
    sum = 0.0;
    ADDP(integral, x0, sum);

    x1 = XVAL(f, 1);
    y1 = FXVAL(f, 1);

    if (n <= 2) {
	/* Fit straight line */
	sum = 0.5 * (x1 - x0) * (y1 + y0);
	ADDP(integral, x1, sum);
    } else {
	/* Fit a curve */
	x2 = XVAL(f, 2);
	y2 = FXVAL(f, 2);

	/* Slope at this point */
	s1 = (y2 - y0) / (x2 - x0);

	/* Slope at first point */
	s0 = (y1 - y0) / (x1 - x0);
	s0 = 2 * s0 - s1;

	/* Calculate step integral */
	del_x = x1 - x0;
	sum += INTEGRAL;
	ADDP(integral, x1, sum);

	for (i = 2; i < n - 1; i++) {
	    /* Crawl forwards */
	    x0 = x1;
	    x1 = x2;
	    x2 = XVAL(f, i + 1);

	    y0 = y1;
	    y1 = y2;
	    y2 = FXVAL(f, i + 1);

	    s0 = s1;
	    s1 = (y2 - y0) / (x2 - x0);

	    /* Calculate step integral */
	    del_x = x1 - x0;
	    sum += INTEGRAL;
	    ADDP(integral, x1, sum);
	}

	/* Final point, no next point */
	x0 = x1;
	x1 = x2;
	y0 = y1;
	y1 = y2;
	s0 = s1;
	s1 = (y1 - y0) / (x1 - x0);
	s1 = 2 * s1 - s0;

	/* Calculate step integral */
	del_x = x1 - x0;
	sum += INTEGRAL;
	ADDP(integral, x1, sum);
    }

    return integral;
}

/*!
  @brief   Set function interpolation type.
  @ingroup func_interp
  @param   f Function.
  @param   type Interpolation type.
*/
void
vf_interpolate(vfunc *f, enum v_interptype type)
{
    VF_CHECK(f);

    if (f->interp != type) {
        f->interp = type;
        MOD(f) = 1;
    }
}

/*!
  @brief   Return maximum X value of a function.
  @ingroup func_access
  @param   f Function.
  @return  Maximum value.
*/
double
vf_maximum(vfunc *f)
{
    VF_CHECK(f);
    vf_update(f, 0);
    return (EMPTY(f) ? 0.0 : XMAX(f));
}

/*!
  @brief   Return minimum X value of a function.
  @ingroup func_access
  @param   f Function.
  @return  Minimum value.
*/
double
vf_minimum(vfunc *f)
{
    VF_CHECK(f);
    vf_update(f, 0);
    return (EMPTY(f) ? 0.0 : XMIN(f));
}

/*!
  @brief   Iterate over a function.
  @ingroup func_access
  @param   iter Iterator.
  @return  Whether to continue.
  @see     v_iter(), v_next(), v_iterate()

  Iterate over the points in a function.  Returns whether there are any
  more points.  The iterator must have been initialized via v_iter() with a
  function object.

  After this call, the iterator structure will contain data on the next
  function point, which you can access using vf_iter_x() and vf_iter_fx().
*/
int
vf_next(viter *iter)
{
    vfunc *f = iter->object;

    VF_CHECK(f);
    vf_update(f, 0);

    if (iter->ipos < NP(f)) {
        iter->fval[0] = XVAL(f, iter->ipos);
        iter->fval[1] = FXVAL(f, iter->ipos);
        iter->ipos++;
        return 1;
    }

    return 0;
}

/*!
  @brief   Return no. of points in a function.
  @ingroup func_access
  @param   f Function.
  @return  No. of points.
*/
int
vf_point_count(vfunc *f)
{
    VF_CHECK(f);
    vf_update(f, 0);
    return NP(f);
}

/*!
  @brief   Return a list of function FX points.
  @ingroup func_access
  @param   f Function.
  @return  List of FX points.
*/
vlist *
vf_points_fx(vfunc *f)
{
    vlist *list;
    int i;

    VF_CHECK(f);

    vf_update(f, 0);
    list = vl_create();

    for (i = 0; i < NP(f); i++)
        vl_dpush(list, FXVAL(f, i));

    return list;
}

/*!
  @brief   Return a list of function X points.
  @ingroup func_access
  @param   f Function.
  @return  List of X points.
*/
vlist *
vf_points_x(vfunc *f)
{
    vlist *list;
    int i;

    VF_CHECK(f);

    vf_update(f, 0);
    list = vl_create();

    for (i = 0; i < NP(f); i++)
        vl_dpush(list, XVAL(f, i));

    return list;
}

/* Print contents of a function */
void
vf_print(vfunc *f, FILE *fp)
{
    int i;

    VF_CHECK(f);
    vf_update(f, 0);

    v_print_start();
    v_push_indent();

    v_print_type(vfunc_type, f, fp);

    for (i = 0; i < NP(f); i++) {
        v_indent(fp);
        fprintf(fp, FLOAT_FORMAT " => " FLOAT_FORMAT "\n",
                XVAL(f, i), FXVAL(f, i));
    }

    v_pop_indent();
    v_print_finish();
}

/* Read function from a stream */
vfunc *
vf_read(FILE *fp)
{
    int num, size;
    double x, fx;
    vfunc *f;

    /* Read initial function size */
    if (!v_read_long(&size, fp))
        return NULL;

    /* Read no. of points */
    if (!v_read_long(&num, fp))
        return NULL;

    /* Read points */
    f = vf_create_size(size);

    while (num-- > 0) {
        if (!v_read_double(&x, fp))
            return NULL;
        if (!v_read_double(&fx, fp))
            return NULL;
        ADDP(f, x, fx);
    }

    return f;
}

/*!
  @brief   Reduce function by removing redundant points.
  @ingroup func_modify
  @param   f Function.
  @param   tolerance Maximum angular difference (deg).
  @return  Reduced function (or \c NULL if no reduction done).

  This works by recursively removing every second point if the angular
  difference between neighbouring points is below a tolerance.  The first
  and last points are never removed.
*/
vfunc *
vf_reduce(vfunc *f, double tolerance)
{
    double x1, x2, x3, fx1, fx2, fx3, a1, a2;
    int i, count;
    vfunc *r;

    VF_CHECK(f);

    /* Need at least 3 points */
    if (NP(f) < 3)
        return NULL;

    /* Create reduced function */
    r = vf_copy(f);
    vf_update(r, 0);
    tolerance *= M_PI / 180.0;

    while (NP(r) > 2) {
        count = 0;

        for (i = NP(r) - 2; i > 0; i--) {
            /* Get angular difference */
            x1 = XVAL(r, i - 1);
            x2 = XVAL(r, i);
            x3 = XVAL(r, i + 1);

            fx1 = FXVAL(r, i - 1);
            fx2 = FXVAL(r, i);
            fx3 = FXVAL(r, i + 1);

            a1 = atan2(fx2 - fx1, x2 - x1);
            a2 = atan2(fx3 - fx2, x3 - x2);

            /* If small enough, remove point */
            if (V_ABS(a1 - a2) < tolerance) {
                count++;
                DELP(r, i);
            }
        }

        /* Continue if points were removed */
        if (count <= 0)
	    break;
    }

    /* If no points removed, do nothing */
    if (NP(r) == NP(f)) {
        vf_destroy(r);
        r = NULL;
    }

    return r;
}

/*!
  @brief   Set resolution of function X values.
  @ingroup func_modify
  @param   f Function.
  @param   res Resolution (must be non-negative).
*/
void
vf_resolution(vfunc *f, double res)
{
    VF_CHECK(f);
    RES(f) = V_MAX(res, 0.0);
}

/* Search function for interpolation point */
static int
vf_search(vfunc *f, double x, int *entry, double *fraction)
{
    int low = 0, mid, high = NP(f) - 1;

    if (high <= 0) {
        /* Not enough points */
        *entry = low;
        *fraction = 0.0;
        return 1;
    } else if (x <= XVAL(f, low)) {
        /* Before first point */
        *entry = low;
        *fraction = 0.0;
        return 1;
    } else if (x >= XVAL(f, high)) {
        /* After last point */
        *entry = high;
        *fraction = 0.0;
        return 1;
    }

    /* Find point with a binary search */
    while (high > low + 1) {
        mid = (low + high) / 2;
        if (x >= XVAL(f, mid))
            low = mid;
	else
            high = mid;
    }

    *entry = low;
    *fraction = (x - XVAL(f, low)) / (XVAL(f, high) - XVAL(f, low));

    return 0;
}

/* Search function for interpolation point with extrapolation */
static int
vf_search_extrapolate(vfunc *f, double x, int *entry, double *fraction)
{
    int low = 0, mid, high = NP(f) - 1;

    if (high <= 0) {
        /* Not enough points */
        *entry = 0;
        *fraction = 0.0;
        return 1;
    }

    /* Find point with a binary search */
    while (high > low + 1) {
	mid = (low + high) / 2;
	if (x >= XVAL(f, mid))
	    low = mid;
	else
	    high = mid;
    }

    *entry = low;
    *fraction = (x - XVAL(f, low)) / (XVAL(f, high) - XVAL(f, low));

    return 0;
}

/*!
  @brief   Set function cycle value
  @ingroup func_interp
  @param   f Function.
  @param   cycle Cycle value.

  Set the cycle value for a function.  Interpolated values will "wrap
  around" and remain in the range 0 to \c cycle.  Setting the cycle value
  to zero or a negative value will turn this feature off.
*/
void
vf_set_cycle(vfunc *f, double cycle)
{
    f->cycle = cycle;

    /* Previous spline is now invalid */
    if (f->spline != NULL) {
	vf_destroy_spline(f->spline);
        f->spline = NULL;
    }
}



/* Point sorting function */
static int
vf_sortpoints(const void *v1, const void *v2)
{
    vpoint *p1 = (vpoint *) v1;
    vpoint *p2 = (vpoint *) v2;

    /* If point has been removed, move it to last */
    if (p1->code == 0)
        return 1;
    else if (p2->code == 0)
        return -1;

    /* Sort on X values */
    if (p1->x < p2->x - resolution)
        return -1;
    else if (p1->x > p2->x + resolution)
        return 1;

    /* If the same, use sort code */
    duplicate = 1;
    if (p1->code > p2->code)
        return -1;
    else if (p1->code < p2->code)
        return 1;

    return 0;
}

/*!
  @brief   Set spline interpolation derivatives.
  @ingroup func_interp
  @param   f Function.
  @param   first Derivative at first point.
  @param   last Derivative at last point.

  Set the spline derivatives of the first and last points on a
  function. Defaults are zero. This effectively sets the 'boundary
  conditions' of the spline. Setting these values is usually only necessary
  when attempting to join a spline to another curve neatly.
*/
void
vf_spline_deriv(vfunc *f, double first, double last)
{
    VF_CHECK(f);

    f->sd1 = first;
    f->sd2 = last;
    MOD(f) = 1;
}

/* Thaw a function from a stream */
vfunc *
vf_thaw(FILE *fp)
{
    double x, fx;
    int token;
    vfunc *f;

    v_thaw_start();

    f = vf_create();

    if (v_thaw_peek(fp) == V_TOKEN_ID) {
        v_thaw_token(fp);
        if        (V_STREQ(v_thaw_svalue, "LINEAR")) {
            f->interp = V_INTERP_LINEAR;
        } else if (V_STREQ(v_thaw_svalue, "LAGRANGE")) {
            f->interp = V_INTERP_LAGRANGE;
        } else if (V_STREQ(v_thaw_svalue, "SPLINE")) {
            f->interp = V_INTERP_SPLINE;
        } else if (V_STREQ(v_thaw_svalue, "NEAREST")) {
            f->interp = V_INTERP_NEAREST;
        } else if (V_STREQ(v_thaw_svalue, "EXTRAPOLATE")) {
            f->interp = V_INTERP_EXTRAPOLATE;
        } else {
            v_thaw_err("invalid interpolation type: %s", v_thaw_svalue);
            goto fail;
        }
    }

    if (!v_thaw_follow(fp, '{', "open-brace"))
        goto fail;

    while (1) {
        if (v_thaw_peek(fp) == '}') {
            v_thaw_token(fp);
            break;
        }

        if (!v_thaw_double(fp, &x))
            goto fail;

        if (!v_thaw_double(fp, &fx))
            goto fail;

        vf_add_point(f, x, fx);

        if ((token = v_thaw_token(fp)) == '}') {
            break;
        } else if (token != ',') {
            v_thaw_expected("comma or close-brace");
            goto fail;
        }
    }

    v_thaw_finish();
    return f;

  fail:
    v_thaw_finish();
    vf_destroy(f);
    return NULL;
}

/*!
  @brief   Apply transform to function values.
  @ingroup func_math
  @param   f Function.
  @param   xfunc X transform function (or NULL).
  @param   fxfunc FX transform function (or NULL).
  @return  Transformed function.
*/
vfunc *
vf_transform(vfunc *f,
             double (*xfunc) (double x),
             double (*fxfunc) (double fx))
{
    vfunc *xform;
    double x, fx;
    int i;

    xform = vf_create_size(NP(f));
    TYPE(xform) = TYPE(f);

    for (i = 0; i < NP(f); i++) {
	x = XVAL(f, i);
	fx = FXVAL(f, i);

	if (xfunc != NULL)
	    x = xfunc(x);

	if (fxfunc != NULL)
	    fx = fxfunc(fx);

	ADDP(xform, x, fx);
    }

    return xform;
}

/* Traverse a function */
int
vf_traverse(vfunc *f, int (*func)(void *ptr))
{
    int val;

    VF_CHECK(f);

    if ((val = func(f)) != 0)
        return val;

    v_push_traverse(f);
    v_pop_traverse();

    return 0;
}

/* Update function internals */
static void
vf_update(vfunc *f, int spline)
{
    int i, ndup = 0;
    double xlast;

    /* Do nothing if empty, or not modified */
    if (!MOD(f) || EMPTY(f))
        return;

    /* Flag unmodified */
    MOD(f) = 0;

    /* Sort points into ascending X values */
    duplicate = 0;
    resolution = RES(f);
    SORT(f);

    /* Remove duplicates, if any */
    if (duplicate) {
        xlast = XVAL(f, 0);
        for (i = 1; i < NP(f); i++) {
            if (XVAL(f, i) - xlast <= resolution) {
                CODE(f, i) = 0;
                ndup++;
            }

            xlast = XVAL(f, i);
        }

        SORT(f);
        NP(f) -= ndup;
    }

    /* Update spline parameters if required */
    if (spline) {
        if (f->spline != NULL)
            vf_destroy_spline(f->spline);
        f->spline = vf_create_spline(f);
    }
}

/*!
  @brief   Interpolate a function value using implicit method.
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value(vfunc *f, double x)
{
    VF_CHECK(f);
    vf_update(f, TYPE(f) == V_INTERP_SPLINE);

    switch (TYPE(f)) {
    case V_INTERP_LINEAR:
        return vf_value_linear(f, x);
    case V_INTERP_LAGRANGE:
        return vf_value_lagrange(f, x);
    case V_INTERP_SPLINE:
        return vf_value_spline(f, x);
    case V_INTERP_NEAREST:
        return vf_value_nearest(f, x);
    case V_INTERP_EXTRAPOLATE:
	return vf_value_extrapolate(f, x);
    }
}

/*!
  @brief   Interpolate a function value using linear method, with extrapolation
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value_extrapolate(vfunc *f, double x)
{
    double fx, fxdiff, fraction;
    int extflag, entry;

    VF_CHECK(f);
    vf_update(f, 0);

    if (EMPTY(f))
        return 0.0;

    extflag = vf_search_extrapolate(f, x, &entry, &fraction);
    fx = FXVAL(f, entry);

    if (!extflag) {
	fxdiff = FXVAL(f, entry + 1) - fx;

	if (f->cycle > 0.0)
	    fxdiff = drem(fxdiff, f->cycle);

	fx += fraction * fxdiff;
    }

    CYCLE_TEST(f, fx);
    return fx;
}

/*!
  @brief   Interpolate a function value using Lagrange 4-point method.
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value_lagrange(vfunc *f, double x)
{
    double f0, fx, f2, f3, fraction;
    int entry;

    VF_CHECK(f);
    vf_update(f, 0);

    if (EMPTY(f))
        return 0.0;

    if (NP(f) <= 3) {
        /* Not enough points -- do linear interpolation instead */
        return vf_value_linear(f, x);
    } else if (vf_search(f, x, &entry, &fraction)) {
        /* Not in domain -- return end point */
        fx = FXVAL(f, entry);
    } else {
        /* Don't use points off the end of the function */
        if (entry == 0) {
            entry++;
            fraction--;
        } else while (entry > NP(f) - 3) {
            entry--;
            fraction++;
        }

        /* Do Lagrange interpolation */
        fx = FXVAL(f, entry);
        f0 = FXVAL(f, entry - 1) - fx;
        f2 = FXVAL(f, entry + 1) - fx;
        f3 = FXVAL(f, entry + 2) - fx;

	if (f->cycle > 0.0) {
	    f0 = drem(f0, f->cycle);
	    f2 = drem(f2, f->cycle);
	    f3 = drem(f3, f->cycle);
	}

        fx -= (fraction * (fraction - 1) * (fraction - 2)) * f0 / 6;
        fx -= ((fraction + 1) * fraction * (fraction - 2)) * f2 / 2;
        fx += ((fraction + 1) * fraction * (fraction - 1)) * f3 / 6;
    }

    CYCLE_TEST(f, fx);
    return fx;
}

/*!
  @brief   Interpolate a function value using linear method.
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value_linear(vfunc *f, double x)
{
    double fx, fxdiff, fraction;
    int extflag, entry;

    VF_CHECK(f);
    vf_update(f, 0);

    if (EMPTY(f))
        return 0.0;

    extflag = vf_search(f, x, &entry, &fraction);

    fx  = FXVAL(f, entry);

    if (!extflag) {
	fxdiff = FXVAL(f, entry + 1) - fx;

	if (f->cycle > 0.0)
	    fxdiff = drem(fxdiff, f->cycle);

	fx += fraction * fxdiff;
    }

    CYCLE_TEST(f, fx);
    return fx;
}

/*!
  @brief   Interpolate a function value using nearest point method.
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value_nearest(vfunc *f, double x)
{
    double fx, fraction;
    int extflag, entry;

    VF_CHECK(f);
    vf_update(f, 0);

    if (EMPTY(f))
        return 0.0;

    extflag = vf_search(f, x, &entry, &fraction);

    if (!extflag && fraction > 0.5)
        fx = FXVAL(f, entry + 1);
    else
	fx = FXVAL(f, entry);

    CYCLE_TEST(f, fx);
    return fx;
}

/*!
  @brief   Interpolate a function value using spline method.
  @ingroup func_interp
  @param   f Function.
  @param   x Value to interpolate.
  @return  Interpolated value.
*/
double
vf_value_spline(vfunc *f, double x)
{
    double fx, fxdiff, t, fraction;
    vspline *s;
    int i;

    VF_CHECK(f);

    vf_update(f, 1);

    if (EMPTY(f))
        return 0.0;

    if (f->spline == NULL)
        f->spline = vf_create_spline(f);

    if (NP(f) <= 3) {
        /* Not enough points -- do linear interpolation instead */
        return vf_value_linear(f, x);
    } else if (vf_search(f, x, &i, &fraction)) {
        /* Not in domain -- return end point */
        fx = FXVAL(f, i);
    } else {
        /* Interpolate spline */
        t = (x - XVAL(f, i)) / f->spline->u[i];
        fx  = FXVAL(f, i);
	CYCLE_DIFF(f, fxdiff, FXVAL(f, i + 1),  fx);

        s = f->spline;
        fx += t * fxdiff
            + s->u[i] * s->u[i]
            * (X3X(t) * s->p[i + 1] + X3X(1 - t) * s->p[i]) / 6;
    }

    CYCLE_TEST(f, fx);
    return fx;
}

/* Write function to a stream */
int
vf_write(vfunc *f, FILE *fp)
{
    int i;

    VF_CHECK(f);

    if (!v_write_long(f->initpoints, fp))
        return 0;

    if (!v_write_long(f->numpoints, fp))
        return 0;

    for (i = 0; i < f->numpoints; i++) {
        if (!v_write_double(f->points[i].x, fp))
            return 0;
        if (!v_write_double(f->points[i].fx, fp))
            return 0;
    }

    return 1;
}

/* Dump XML function to a stream */
static int
vf_xmldump(vfunc *f, FILE *fp)
{
    char *tag = NULL, xval[20], fxval[20];
    int i;

    VF_CHECK(f);
    vf_update(f, 0);

    v_xmldump_start(fp);

    switch (f->interp) {

    case V_INTERP_LAGRANGE:
        tag = "LAGRANGE";
        break;

    case V_INTERP_SPLINE:
        tag = "SPLINE";
        break;

    case V_INTERP_NEAREST:
        tag = "NEAREST";
        break;

    case V_INTERP_EXTRAPOLATE:
        tag = "EXTRAPOLATE";
        break;
    }

    if (tag != NULL)
        v_xmldump_tag(fp, "interp", "type", tag, NULL);

    for (i = 0; i < f->numpoints; i++) {
        sprintf(xval, FLOAT_FORMAT, f->points[i].x);
        sprintf(fxval, FLOAT_FORMAT, f->points[i].fx);
        v_xmldump_tag(fp, "point", "x", xval, "fx", fxval, NULL);
    }

    v_xmldump_finish(fp);
    return 1;
}

/* Dump YAML function to a stream */
static int
vf_yamldump(vfunc *f, FILE *fp)
{
    char *tag = NULL, val[20];
    int i;

    VF_CHECK(f);
    vf_update(f, 0);

    if (!v_yaml_start(fp))
        return 0;

    if (!v_yaml_start_hash(fp, "func"))
        return 0;

    switch (f->interp) {

    case V_INTERP_LAGRANGE:
        tag = "LAGRANGE";
        break;

    case V_INTERP_SPLINE:
        tag = "SPLINE";
        break;

    case V_INTERP_NEAREST:
        tag = "NEAREST";
        break;

    case V_INTERP_EXTRAPOLATE:
        tag = "EXTRAPOLATE";
        break;
    }

    if (tag != NULL) {
        if (!v_yaml_write_string("interp", fp))
            return 0;

        if (!v_yaml_write_string(tag, fp))
            return 0;
    }

    if (!v_yaml_write_string("points", fp))
        return 0;

    if (!v_yaml_start_list(fp, NULL))
        return 0;

    for (i = 0; i < f->numpoints; i++) {
        if (!v_yaml_start_list(fp, NULL))
            return 0;

        sprintf(val, FLOAT_FORMAT, f->points[i].x);
        if (!v_yaml_write_string(val, fp))
            return 0;

        sprintf(val, FLOAT_FORMAT, f->points[i].fx);
        if (!v_yaml_write_string(val, fp))
            return 0;

        if (!v_yaml_finish_list(fp))
            return 0;
    }

    if (!v_yaml_finish_list(fp))
        return 0;

    if (!v_yaml_finish_hash(fp))
        return 0;

    if (!v_yaml_finish(fp))
        return 0;

    return 1;
}
