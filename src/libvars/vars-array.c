/*!
  @defgroup array Arrays
  @ingroup types

  An array is an object which contains an ordered list of value-scalar
  pairs.  The value is a real number.
*/

/*!
  @defgroup array_create Creating and destroying arrays
  @ingroup array

  These functions create and destroy arrays.
*/

/*!
  @defgroup array_modify Modifying arrays
  @ingroup array

  These functions are available for adding and removing value-scalar pairs
  from an array.  Note that entries can be added in any order -- when
  required, the pairs are sorted by increasing value.

  When a pair is added to an array, any existing pair with the same value
  is replaced by it.  The definition of 'same value' is any pair with a
  value which differs by less than the resolution of the array.  The
  default resolution is 1.0e-4, but can be changed via va_resolution().

  If you're storing pointers to other objects in array entries, you may
  want to deal with them when their array entries get overwritten (e.g.
  reclaim memory). The function va_remove_func() can do that.
*/

/*!
  @defgroup array_interp Array value interpolation
  @ingroup array

  These functions do linear interpolation to find entries.
*/

/*!
  @defgroup array_access Accessing array information
  @ingroup array

  These functions are available for getting miscellaneous information from
  arrays.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "vars-config.h"
#include "vars-array.h"
#include "vars-macros.h"
#include "vars-memory.h"

#ifndef ARRAY_DEFAULT_SIZE
#define ARRAY_DEFAULT_SIZE 20
#endif

#define FLOAT_FORMAT "%.7g"

/* Various shortcut macros */
#define NP(a)       ((a)->numpoints)
#define INITP(a)    ((a)->initpoints)
#define MAXP(a)     ((a)->maxpoints)
#define MOD(a)      ((a)->modflag)
#define RES(a)      ((a)->resolution)

#define XVAL(a, n)  ((a)->points[n].x)
#define SVAL(a, n)  ((a)->points[n].s)
#define CODE(a, n)  ((a)->points[n].code)

#define XMIN(a)     XVAL(a, 0)
#define XMAX(a)     XVAL(a, NP(a) - 1)
#define EMPTY(a)    (NP(a) == 0)

#define FRACTION(a, x, i)						\
	((x - XVAL(a, i)) / (XVAL(a, i+1) - XVAL(a, i)))

#define NEXT_NEARER(a, x, i)						\
	((x - XVAL(a, i)) > (XVAL(a, i+1) - x))

#define SORT(a) qsort((a)->points, NP(a), sizeof(vpoint), va_sortpoints)

#define VA_UPDATE(A)	if (MOD(a)) va_update(a)

#define ADDP(a, x, s) do {                                      \
    if (NP(a) == MAXP(a) - 1) {                                 \
        MAXP(a) += INITP(a);                                    \
        a->points = V_REALLOC(a->points, vpoint, MAXP(a));      \
    }                                                           \
    if (NP(a) > 0 && XVAL(a, NP(a) - 1) >= x) MOD(a) = 1;       \
    XVAL(a, NP(a)) = x;                                         \
    SVAL(a, NP(a)) = s;                                         \
    CODE(a, NP(a)) = ++sortcode;                                \
    NP(a)++;                                                    \
} while (0)

/* A single point */
struct v_point {
    float x;            /* X value */
    vscalar *s;		/* Table value */
    int code;           /* Sort code */
};

typedef struct v_point vpoint;

/* Type definition */
struct v_array {
    struct v_header id;         /* Type marker */

    struct v_point *points;     /* Function points */
    unsigned int initpoints;    /* Initial point allocation */
    unsigned int maxpoints;     /* Max. points allocated */
    unsigned int numpoints;     /* No. of points used */
    float resolution;           /* Resolution of X values */
    int modflag;                /* Function modified? */

    void (*remove)(vscalar *s); /* Scalar removal function */
};

/* Type variable */
vtype *varray_type = NULL;

/* Sort code values */
static int sortcode = 0;

/* X value resolution */
static float resolution;

/* Duplicate-point flag */
static int duplicate;

/* Internal functions */
static int va_search(varray *a, float x, int *entry);
static int va_sortpoints(const void *v1, const void *v2);
static void va_update(varray *a);
static int va_xmldump(varray *a, FILE *fp);

/*!
  @brief   Add a list of value-scalar pairs to an array.
  @ingroup array_modify
  @param   a Array.
  @param   x List of X values to add.
  @param   s List of scalars to add.
*/
void
va_add_list(varray *a, vlist *x, vlist *s)
{
    int i, num;

    VA_CHECK(a);
    VL_CHECK(x);
    VL_CHECK(s);

    num = V_MIN(vl_length(x), vl_length(s));

    for (i = 0; i < num; i++)
	ADDP(a, vl_dget(x, i), vs_copy(vl_get(s, i)));
}

/*!
  @brief   Build an array from a list of arguments.
  @ingroup array_create
  @return  New array.

  Create and return an array containing a set of double-value pairs.  Each
  array entry is specified by three parameters: a <tt>double</tt> array
  value, an \c int type, and a value depending on its type.  The type can
  be one of V_TYPE_INT, V_TYPE_FLOAT, V_TYPE_DOUBLE, V_TYPE_STRING or
  V_TYPE_POINTER.  The type of the following value should correspond to
  this, and the list of key-value pairs should be terminated by VA_END.  If
  a value has the wrong type, or no terminator is supplied, chaos will
  surely result.
*/
varray *
va_array(double x, ...)
{
    int type, ival;
    double dval;
    void *pval;
    va_list ap;
    float fval;
    char *sval;
    varray *a;

    a = va_create();
    va_start(ap, x);

    while (x != VA_END) {
        type = va_arg(ap, int);

        switch (type) {

        case V_TYPE_INT:
            ival = va_arg(ap, int);
            va_istore(a, x, ival);
            break;

        case V_TYPE_FLOAT:
            fval = va_arg(ap, double);
            va_fstore(a, x, fval);
            break;

        case V_TYPE_DOUBLE:
            dval = va_arg(ap, double);
            va_dstore(a, x, dval);
            break;

        case V_TYPE_STRING:
            sval = va_arg(ap, char *);
            va_sstore(a, x, sval);
            break;

        case V_TYPE_POINTER:
            pval = va_arg(ap, void *);
            va_pstore(a, x, pval);
            break;

        case V_TYPE_UNDEF:
            va_store(a, x, vs_create(V_TYPE_UNDEF));
            break;
        }

        x = va_arg(ap, double);
    }

    va_end(ap);
    return a;
}

/* Check if pointer is an array */
int
va_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == varray_type);
}

/*!
  @brief   Return a copy of an array.
  @ingroup array_create
  @param   a Array.
  @return  A copy of the array.
*/
varray *
va_copy(varray *a)
{
    varray *copy;
    int i;

    VA_CHECK(a);

    copy = va_create_size(INITP(a));

    for (i = 0; i < NP(a); i++)
	ADDP(copy, XVAL(a, i), vs_copy(SVAL(a, i)));

    RES(copy) = RES(a);

    return copy;
}

/*!
  @brief   Create and return a new (empty) array.
  @ingroup array_create
  @return  New array.
*/
varray *
va_create(void)
{
    return va_create_size(ARRAY_DEFAULT_SIZE);
}

/*!
  @brief   Create a new (empty) array with the given initial size.
  @ingroup array_create
  @param   size Initial size of the array.
  @return  New array.
*/
varray *
va_create_size(unsigned int size)
{
    static vheader *id = NULL;
    varray *a;

    if (id == NULL) {
	va_declare();
	id = v_header(varray_type);
    }

    a = V_ALLOC(varray, 1);
    a->id = *id;

    a->points = V_ALLOC(vpoint, size);
    a->initpoints = V_MAX(size, 1);
    a->maxpoints = V_MAX(size, 1);
    a->numpoints = 0;
    a->modflag = 0;
    a->resolution = 1.0e-4;
    a->remove = NULL;

    return a;
}

/* Declare array type */
vtype *
va_declare(void)
{
    if (varray_type == NULL) {
	varray_type = v_create("ARRAY", "A");
	v_create_func(varray_type, (void *(*)()) va_create);
	v_copy_func(varray_type, (void *(*)()) va_copy);
	v_next_func(varray_type, va_next);
	v_read_func(varray_type, (void *(*)()) va_read);
	v_write_func(varray_type, va_write);
	v_freeze_func(varray_type, va_freeze);
	v_thaw_func(varray_type, (void *(*)()) va_thaw);
	v_print_func(varray_type, va_print);
	v_xmldump_func(varray_type, va_xmldump);
	v_destroy_func(varray_type, va_destroy);
	v_traverse_func(varray_type, va_traverse);
    }

    return varray_type;
}

/*!
  @brief   Return whether an interpolation value is defined.
  @ingroup array_access
  @param   a Array.
  @param   x Value to check.
  @return  Yes or no.

  Return whether an array is defined at a given value (i.e. whether the
  value is between the minimum and maximum values). This is always false
  for an empty array.
*/
int
va_defined(varray *a, float x)
{
    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a))
	return 0;

    return (x >= XMIN(a) && x <= XMAX(a));
}

/*!
  @brief   Delete an array entry.
  @ingroup array_modify
  @param   a Array to modify.
  @param   num The index of the entry to delete.
  @return  Whether anything was deleted.
*/
int
va_delete(varray *a, int num)
{
    VA_CHECK(a);

    if (num < 0 || num >= NP(a))
	return 0;

    XVAL(a, num) = XVAL(a, NP(a) - 1);
    SVAL(a, num) = SVAL(a, NP(a) - 1);
    CODE(a, num) = CODE(a, NP(a) - 1);
    MOD(a) = 1;
    NP(a)--;

    return 1;
}

/*!
  @brief   Deallocate an array.
  @ingroup array_create
  @param   a Array to destroy.
*/
void
va_destroy(varray *a)
{
    int i;

    VA_CHECK(a);

    for (i = 0; i < NP(a); i++)
        vs_destroy(SVAL(a, i));

    V_DEALLOC(a->points);
    V_DEALLOC(a);
}

/*!
  @brief   Empty an array of all points.
  @ingroup array_modify
  @param   a Array to empty.
*/
void
va_empty(varray *a)
{
    VA_CHECK(a);
    NP(a) = 0;
}

/*!
  @brief   Find the two nearest values for interpolation purposes.
  @ingroup array_interp
  @param   a Array.
  @param   x Value to find.
  @param[out]   before Scalar before the value.
  @param[out]   after Scalar after the value.
  @param[out]   fraction Interpolation fraction.
  @return  Number of scalars returned.
  @retval  0 if array is empty, 1 if the array is constant, or 2 if the
	   array varies.

  Find the two nearest scalars to the specified value.  Unlike va_interp()
  this function is useful when extrapolation is permitted.
*/
int
va_extrapolate(varray *a, float x, vscalar **before,
	       vscalar **after, float *fraction)
{
    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a)) {
	*before = *after = NULL;
	return 0;
    } else if (NP(a) == 1) {
	/* Only one point */
	*before = *after = SVAL(a, 0);
	*fraction = 0.0;
	return 1;
    } else {
	int low = 0, mid, high = NP(a) - 1;

	/* Find point with a binary search */
	while (high > low + 1) {
	    mid = (low + high) / 2;
	    if (x >= XVAL(a, mid))
		low = mid;
	    else
		high = mid;
	}

	*before = SVAL(a, low);
	*after  = SVAL(a, high);
	*fraction = FRACTION(a, x, low);
	return 2;
    }
}

/*!
  @brief   Find a place in an array.
  @ingroup array_access
  @param   a Array.
  @param   x Value to find.
  @return  Entry index.
  @retval  -1 if not present (within array's resolution).
*/
int
va_find(varray *a, float x)
{
    int entry = va_find_nearest(a, x);

    /* Is it close enough? */
    if (fabs(x - XVAL(a, entry)) > RES(a))
	return -1;

    /* Found it */
    return entry;
}

/*!
  @brief   Find nearest scalar to a value.
  @ingroup array_interp
  @param   a Array.
  @param   x Value to find.
  @return  Entry index.
*/
int
va_find_nearest(varray *a, float x)
{
    int entry, extflag;

    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a))
	return -1;

    /* Find nearest point */
    extflag = va_search(a, x, &entry);
    if (!extflag && NEXT_NEARER(a, x, entry))
	entry++;

    /* Found it */
    return entry;
}

/* Freeze an array to a stream */
int
va_freeze(varray *a, FILE *fp)
{
    int i;

    VA_CHECK(a);
    VA_UPDATE(a);

    v_freeze_start(fp);

    fputs("{\n", fp);
    v_push_indent();

    for (i = 0; i < NP(a); i++) {
	v_indent(fp);
	fprintf(fp, FLOAT_FORMAT, XVAL(a, i));
	fputs(" = ", fp);

	if (vs_freeze(SVAL(a, i), fp))
	    fputs(",\n", fp);
        else
	    return 0;
    }

    v_pop_indent();
    v_indent(fp);
    fputc('}', fp);

    v_freeze_finish(fp);

    return 1;
}

/*!
  @brief   Find nearest scalar to a value.
  @ingroup array_interp
  @param   a Array.
  @param   x Value to find.
  @return  Nearest scalar.
  @retval  NULL if array is empty.
*/
vscalar *
va_get(varray *a, float x)
{
    int extflag, entry;

    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a))
	return NULL;

    extflag = va_search(a, x, &entry);

    if (!extflag && NEXT_NEARER(a, x, entry))
        entry++;

    return SVAL(a, entry);
}

/*!
  @brief   Find point exactly.
  @ingroup array_interp
  @param   a Array.
  @param   x Value to find.
  @return  Scalar.
  @retval  NULL if no exact match.

  Like va_get(), but only an "exact" match (i.e. within the resolution of
  the array).
*/
vscalar *
va_get_exact(varray *a, float x)
{
    int extflag, entry;

    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a))
	return NULL;

    extflag = va_search(a, x, &entry);

    if (!extflag && NEXT_NEARER(a, x, entry))
	entry++;

    if (fabs(x - XVAL(a, entry)) > RES(a))
	return NULL;

    return SVAL(a, entry);
}

/*!
  @brief   Return a point in an array.
  @ingroup array_access
  @param   a Array.
  @param   n Entry number.
  @param[out]   x Value.
  @param[out]   s Scalar.
  @return  Whether \c n is valid.

  Get the Nth value-scalar pair in an array, counting from zero.
*/
int
va_get_point(varray *a, int n, float *x, vscalar **s)
{
    VA_CHECK(a);

    if (n < 0 || n >= NP(a))
	return 0;

    VA_UPDATE(a);
    *x = XVAL(a, n);
    *s = SVAL(a, n);

    return 1;
}

/*!
  @brief   Find the two nearest values for interpolation purposes.
  @ingroup array_interp
  @param   a Array.
  @param   x Value to find.
  @param[out]   before Scalar before the value.
  @param[out]   after Scalar after the value.
  @param[out]   fraction Interpolation fraction.
  @return  Number of scalars returned.
  @retval  0 if array is empty.

  Find the two nearest scalars to the specified value. If the value
  is outside the range of the array, only \c before is set.
*/
int
va_interp(varray *a, float x, vscalar **before,
	  vscalar **after, float *fraction)
{
    int extflag, entry;

    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a)) {
	*before = *after = NULL;
	return 0;
    }

    extflag = va_search(a, x, &entry);
    *before = SVAL(a, entry);

    if (extflag) {
	/* Extrapolating, only one point */
	*after = NULL;
	*fraction = 0.0;
	return 1;
    } else {
	/* Interpolating, two points */
	*after = SVAL(a, entry + 1);
	*fraction = FRACTION(a, x, entry);
	return 2;
    }
}

/*!
  @brief   Find values for interpolation purposes.
  @ingroup array_interp

  @param   a Array.
  @param   x Value to find.
  @param   method Chosen interpolation method, see va_interptype
  @param[out]   points Scalars around the value.
  @param[out]   fractions Fractions of scalars to use for your
			  chosen interpolation method
  @return  Whether extrapolation was required
  @retval  -1 if array is empty.

  General purpose array iterpolation function which will do interpolation,
  extrapolation, and some curve fitting; at your choice.
*/
int
va_lookup(varray *a, float x, enum v_interptype method,
	  vscalar *points[4], float fractions[4])
{
    int extflag, entry;
    float fraction;
    int i, j, nl;

    /* Zero everything */
    V_ZERO(points, 4);
    V_ZERO(fractions, 4);

    VA_CHECK(a);
    VA_UPDATE(a);

    if (EMPTY(a))
	return -1;

    if (NP(a) <= 1) {
	/* Single point, must be right for all ocassions */
	points[0] = SVAL(a, 0);
	fractions[0] = 1.0;

	/* If you only have one point, you are never extrapolating */
	return 0;
    }

    /* Make sure we have enough points to implement lagrange 4 point
     * interpolation  */
    if (method == V_INTERP_LAGRANGE && NP(a) < 3)
	method = V_INTERP_LINEAR;

    /* Find the point in the array */
    extflag = va_search(a, x, &entry);

    if (extflag) {
	switch (method) {

	case V_INTERP_LINEAR:
	case V_INTERP_NEAREST:
	case V_INTERP_LAGRANGE:
	    /* These interpolation methods flatline when extrapolating */
	    points[0] = SVAL(a, entry);
	    fractions[0] = 1.0;
	    return 1;

	case V_INTERP_EXTRAPOLATE:
	    /* These methods extrapolate */
	    if (entry + 1 >= NP(a))
		entry--;
	}
    }

    switch (method) {

    case V_INTERP_NEAREST:
	if (NEXT_NEARER(a, x, entry))
	    entry++;
	points[0] = SVAL(a, entry);
	fractions[0] = 1.0;
	break;

    case V_INTERP_LINEAR:
    case V_INTERP_EXTRAPOLATE:
	/* All above methods are the same if not extrapolating */
	points[0] = SVAL(a, entry);
	points[1] = SVAL(a, entry + 1);

	fraction = FRACTION(a, x, entry);
	fractions[0] = 1.0 - fraction;
	fractions[1] = fraction;
	break;

    case V_INTERP_LAGRANGE:
    {
	float xes[4];

	/* Use three or four point interpolation? */
        nl = (NP(a) >= 4) ? 4 : 3;

	/* Don't use points off the end of the function */
	if (entry == 0) {
	    entry++;
	} else while (entry >= NP(a) - nl) {
	    entry--;
	}

	for (i = 0; i < nl; i++) {
	    points[i] = SVAL(a, entry + i - 1);
	    xes[i]    = XVAL(a, entry + i - 1);
	}

	for (i = 0; i < nl; i++) {
	    float f = 1.0;
	    for (j = 0; j < nl; j++) {
		if (j != i)
		    f *= (x - xes[j]) / (xes[i] - xes[j]);
	    }

	    fractions[i]  = f;
	}
    }
    break;

    case V_INTERP_SPLINE:
        v_fatal("va_lookup(): spline interpolation not implemented");
    }

    return extflag;
}

/*!
  @brief   Return maximum value of an array.
  @ingroup array_access
  @param   a Array.
  @return  Maximum value.
  @retval  0 if array is empty.
*/
float
va_maximum(varray *a)
{
    VA_CHECK(a);
    return (EMPTY(a) ? 0.0 : XMAX(a));
}

/*!
  @brief   Return minimum value of an array.
  @ingroup array_access
  @param   a Array.
  @return  Minimum value.
  @retval  0 if array is empty.
*/
float
va_minimum(varray *a)
{
    VA_CHECK(a);
    return (EMPTY(a) ? 0.0 : XMIN(a));
}

/*!
  @brief   Iterate over an array.
  @ingroup hash_access
  @param   iter Iterator.
  @return  Whether to continue.
  @see     v_iter(), v_next(), v_iterate()

  Iterate over the float-value pairs in an array.  Returns whether there
  are no more entries.  The iterator must have been initialized via
  v_iter() with an array object.

  After this call, the iterator structure will contain data on the next
  array entry, which you can access using va_iter_x() and va_iter_val().
*/
int
va_next(viter *iter)
{
    varray *a = iter->object;

    VA_CHECK(a);
    VA_UPDATE(a);

    if (iter->ipos < NP(a)) {
        iter->fval[0] = XVAL(a, iter->ipos);
        iter->pval[0] = SVAL(a, iter->ipos);
        iter->ipos++;
	return 1;
    }

    return 0;
}

/*!
  @brief   Return no. of points in an array.
  @ingroup array_access
  @param   a Array.
  @return  No. of points.
*/
int
va_point_count(varray *a)
{
    VA_CHECK(a);
    VA_UPDATE(a);

    return NP(a);
}

/*!
  @brief   Return a list of array scalars.
  @ingroup array_access
  @param   a Array.
  @return  List of scalars.
*/
vlist *
va_points_s(varray *a)
{
    vlist *list;
    int i;

    VA_CHECK(a);
    VA_UPDATE(a);

    list = vl_create();

    for (i = 0; i < NP(a); i++)
	vl_push(list, vs_copy(SVAL(a, i)));

    return list;
}

/*!
  @brief   Return a list of array values.
  @ingroup array_access
  @param   a Array.
  @return  List of values.
*/
vlist *
va_points_x(varray *a)
{
    vlist *list;
    int i;

    VA_CHECK(a);
    VA_UPDATE(a);

    list = vl_create();

    for (i = 0; i < NP(a); i++)
	vl_dpush(list, XVAL(a, i));

    return list;
}

/* Print contents of an array */
void
va_print(varray *a, FILE *fp)
{
    int i;

    VA_CHECK(a);
    VA_UPDATE(a);

    v_print_start();
    v_push_indent();

    v_print_type(varray_type, a, fp);

    for (i = 0; i < NP(a); i++) {
	v_indent(fp);
	fprintf(fp, FLOAT_FORMAT " => ", XVAL(a, i));
	v_print(SVAL(a, i), fp);
    }

    v_pop_indent();
    v_print_finish();
}

/* Read array from a stream */
varray *
va_read(FILE *fp)
{
    int num, size;
    vscalar *s;
    varray *a;
    float x;

    /* Read initial array size */
    if (!v_read_long(&size, fp))
	return NULL;

    /* Read no. of points */
    if (!v_read_long(&num, fp))
	return NULL;

    /* Read points */
    a = va_create_size(size);

    while (num-- > 0) {
	/* Read key */
	if (!v_read_float(&x, fp))
	    return NULL;
	/* Read value */
	if ((s = vs_read(fp)) == NULL)
	    return NULL;

	ADDP(a, x, s);
    }

    return a;
}

/*!
  @brief   Set an array scalar removal function.
  @ingroup array_modify
  @param   a Array.
  @param   remove Removal function (or \c NULL ).

  Define a scalar removal function for the given array. The function is
  called whenever a scalar is about to be removed from the array, giving
  you the chance to do whatever you want with it. If a removal function is
  not defined, the default action is just to destroy the scalar. Note that
  destruction doesn't happen if there \e is a removal function defined.
*/
void
va_remove_func(varray *a, void (*remove)(vscalar *s))
{
    VA_CHECK(a);
    a->remove = remove;
}

/*!
  @brief   Set resolution of array values.
  @ingroup array_modify
  @param   a Array to set.
  @param   res The resolution.

  Set the resolution of an array to the given (non-negative) value.
*/
void
va_resolution(varray *a, float res)
{
    VA_CHECK(a);
    RES(a) = V_MAX(res, 0.0);
}

/* Search array for interpolation point */
static int
va_search(varray *a, float x, int *entry)
{
    int low = 0, mid, high = NP(a) - 1;

    if (high <= 0) {
	/* Not enough points */
	*entry = low;
	return 1;
    } else if (x <= XVAL(a, low)) {
	/* Before first point */
	*entry = low;
	return 1;
    } else if (x >= XVAL(a, high)) {
	/* After last point */
	*entry = high;
	return 1;
    }

    /* Find point with a binary search */
    while (high > low + 1) {
	mid = (low + high) / 2;
	if (x >= XVAL(a, mid))
	    low = mid;
	else
	    high = mid;
    }

    *entry = low;

    return 0;
}

/* Point sorting array */
static int
va_sortpoints(const void *v1, const void *v2)
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
  @brief   Add a value-scalar pair to an array.
  @ingroup array_modify
  @param   a Array to add to.
  @param   x Value to add.
  @param   s Scalar to add (not copied).
*/
void
va_store(varray *a, float x, vscalar *s)
{
    VA_CHECK(a);
    VS_CHECK(s);
    ADDP(a, x, s);
}

/* Thaw an array from a stream */
varray *
va_thaw(FILE *fp)
{
    double x;
    int token;
    varray *a;
    vscalar *s;

    v_thaw_start();
    a = va_create();

    if (!v_thaw_follow(fp, '{', "open-brace"))
	goto fail;

    while (1) {
	if (v_thaw_peek(fp) == '}') {
	    v_thaw_token(fp);
	    break;
	}

	if (!v_thaw_double(fp, &x))
	    goto fail;

	if (!v_thaw_follow(fp, '=', "'='"))
	    goto fail;

	if ((s = vs_thaw(fp)) == NULL)
	    goto fail;

	va_store(a, (float) x, s);

	token = v_thaw_token(fp);
	if (token == '}') {
	    break;
	} else if (token != ',') {
	    v_thaw_expected("comma or close-brace");
	    goto fail;
	}
    }

    v_thaw_finish();
    return a;

  fail:
    v_thaw_finish();
    v_destroy(a);
    return NULL;
}

/* Traverse an array */
int
va_traverse(varray *a, int (*func) (void *ptr))
{
    int i, val;
    vscalar *s;

    VA_CHECK(a);

    if ((val = func(a)) != 0)
	return val;

    if (v_traverse_seen(a))
	return 0;

    v_push_traverse(a);

    for (i = 0; i < NP(a); i++) {
	s = SVAL(a, i);
	if (vs_type(s) == V_TYPE_POINTER) {
	    val = v_traverse(vs_pget(s), func);
	    if (val != 0) {
		v_pop_traverse();
		return val;
	    }
	}
    }

    v_pop_traverse();

    return 0;
}

/* Update array internals */
static void
va_update(varray *a)
{
    int i, ndup = 0;
    float xlast;

    /* Flag unmodified */
    MOD(a) = 0;

    /* Do nothing if empty, or not modified */
    if (EMPTY(a))
	return;

    /* Sort points into ascending X values */
    duplicate = 0;
    resolution = RES(a);
    SORT(a);

    /* Remove duplicates, if any */
    if (duplicate) {
	xlast = XVAL(a, 0);
	for (i = 1; i < NP(a); i++) {
	    if (XVAL(a, i) - xlast <= resolution) {
		CODE(a, i) = 0;

                if (a->remove != NULL)
                    a->remove(SVAL(a, i));
                else
                    vs_destroy(SVAL(a, i));

		ndup++;
	    }

	    xlast = XVAL(a, i);
	}

	SORT(a);
	NP(a) -= ndup;
    }
}

/* Write array to a stream */
int
va_write(varray *a, FILE *fp)
{
    int i;

    VA_CHECK(a);

    if (!v_write_long(a->initpoints, fp))
	return 0;

    if (!v_write_long(a->numpoints, fp))
	return 0;

    for (i = 0; i < a->numpoints; i++) {
	if (!v_write_float(XVAL(a, i), fp))
	    return 0;
	if (!vs_write(SVAL(a, i), fp))
	    return 0;
    }

    return 1;
}

/* Dump XML array to a stream */
static int
va_xmldump(varray *a, FILE *fp)
{
    char buf[20];
    int i;

    VA_CHECK(a);
    VA_UPDATE(a);

    v_xmldump_start(fp);

    for (i = 0; i < NP(a); i++) {
	sprintf(buf, FLOAT_FORMAT, XVAL(a, i));

        if (vs_defined(SVAL(a, i))) {
            v_xmldump_tag_start(fp, "entry", "x", buf, NULL);

            if (vs_xmldump(SVAL(a, i), fp))
                v_xmldump_tag_finish(fp, "entry");
            else
                return 0;
        } else {
            v_xmldump_tag(fp, "entry", "x", buf, NULL);
        }
    }

    v_xmldump_finish(fp);
    return 1;
}
