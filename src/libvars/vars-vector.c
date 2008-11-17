/*!
  @defgroup vector Vectors
  @ingroup types

  A vector is an ordered list of floats.  Once a vector is created, its
  size (which must be greater than zero) cannot change.  Vector elements
  are numbered from zero.
*/

/*!
  @defgroup vector_create Creating and destroying vectors
  @ingroup vector
*/

/*!
  @defgroup vector_access Accessing vector elements
  @ingroup vector
*/

/*!
  @defgroup vector_modify Modifying vector elements
  @ingroup vector
*/

/*!
  @defgroup vector_misc Other vector functions
  @ingroup vector
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vars-config.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-vector.h"

#define ALLOCSIZE 10

/* Type definition */
struct v_vector {
    struct v_header id;         /* Type marker */
    unsigned int size;          /* Size */
    float *data;                /* Values */
};

/* Type variable */
vtype *vvector_type = NULL;

/* Vector freeze format */
static char *freeze_format = "%g";

/* No. of elements per freeze line */
static int freeze_linesize = 5;

/* Vector print format */
static char *print_format = "%g";

/* No. of elements per print line */
static int print_linesize = 5;

/* Internal functions */
static int vv_sortpoints(const void *v1, const void *v2);
static int vv_xmldump(vvector *v, FILE *fp);

/* Check if pointer is a vector */
int
vv_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vvector_type);
}

/*!
  @brief   Return a copy of a vector.
  @ingroup vector_create
  @param   v Vector.
  @return  Copy.
*/
vvector *
vv_copy(vvector *v)
{
    VV_CHECK(v);

    return vv_create_from_values(v->size, v->data);
}

/*!
  @brief   Create a zeroised vector.
  @ingroup vector_create
  @param   size Length of vector.
  @return  Vector.
*/
vvector *
vv_create(unsigned int size)
{
    return vv_create_from_values(size, NULL);
}

/*!
  @brief   Create a vector from a list.
  @ingroup vector_create
  @param   list List of values.
  @return  Vector.

  The vector size is the length of the list, and each element is the float
  value of the corresponding list element.  The list must have at least one
  element.
*/
vvector *
vv_create_from_list(vlist *list)
{
    unsigned int i, size;
    vvector *v;

    VL_CHECK(list);

    if ((size = vl_length(list)) == 0)
        v_fatal("vv_create_from_list(): zero length list");

    v = vv_create_with_values(size, NULL);

    for (i = 0; i < size; i++)
        v->data[i] = vl_fget(list, i);

    return v;
}

/*!
  @brief   Create a vector by copying the given values.
  @ingroup vector_create
  @param   size No. of values.
  @param   values List of values.
  @return  Vector.
*/
vvector *
vv_create_from_values(unsigned int size, float *values)
{
    vvector *v;
    int i;

    if (size == 0)
        v_fatal("vv_create_from_values(): zero size vector");

    v = vv_create_with_values(size, NULL);

    for (i = 0; i < size; i++)
        v->data[i] = (values == NULL ? 0.0 : values[i]);

    return v;
}

/*!
  @brief   Create a vector from explicit values.
  @ingroup vector_create
  @param   size Length of vector.
  @return  Vector.

  There must be exactly \c size remaining arguments, which are the values
  of the vector elements.
*/
vvector *
vv_create_vector(unsigned int size, ...)
{
    float *values;
    va_list ap;
    int i;

    if (size != 0)
        values = V_ALLOC(float, size);
    else
        v_fatal("vv_create_vector(): zero size vector");

    va_start(ap, size);

    for (i = 0; i < size; i++)
        values[i] = va_arg(ap, double);

    va_end(ap);

    return vv_create_with_values(size, values);
}

/*!
  @brief   Create a new vector using the given values.
  @ingroup vector_create
  @param   size No. of values.
  @param   values List of values (deallocated along with the vector).
  @return  Vector.
*/
vvector *
vv_create_with_values(unsigned int size, float *values)
{
    static vheader *id = NULL;
    vvector *v;

    if (size == 0)
        v_fatal("vv_create_with_values(): zero size vector");

    if (id == NULL) {
        vv_declare();
        id = v_header(vvector_type);
    }

    v = V_ALLOC(vvector, 1);
    v->id = *id;

    v->size = size;

    if (values != NULL)
        v->data = values;
    else
        v->data = V_ALLOC(float, size);

    return v;
}

/* Declare vector type */
vtype *
vv_declare(void)
{
    if (vvector_type == NULL) {
        vvector_type = v_create("VECTOR", "V");
        v_copy_func(vvector_type, (void *(*)()) vv_copy);
        v_read_func(vvector_type, (void *(*)()) vv_read);
        v_write_func(vvector_type, vv_write);
        v_freeze_func(vvector_type, vv_freeze);
        v_thaw_func(vvector_type, (void *(*)()) vv_thaw);
        v_print_func(vvector_type, vv_print);
        v_destroy_func(vvector_type, vv_destroy);
        v_traverse_func(vvector_type, vv_traverse);
        v_xmldump_func(vvector_type, vv_xmldump);
    }

    return vvector_type;
}

/*!
  @brief   Deallocate a vector.
  @ingroup vector_create
  @param   v Vector.
*/
void
vv_destroy(vvector *v)
{
    VV_CHECK(v);

    V_DEALLOC(v->data);
    V_DEALLOC(v);
}

/* Freeze vector to a stream */
int
vv_freeze(vvector *v, FILE *fp)
{
    int from = 0, to, i;

    VV_CHECK(v);

    v_freeze_start(fp);

    fputs("{\n", fp);
    v_push_indent();

    while (from < v->size) {
        to = V_MIN(from + freeze_linesize, v->size);
        v_indent(fp);

        for (i = from; i < to; i++) {
            fprintf(fp, freeze_format, v->data[i]);
            fputc(',', fp);

            if (i < to - 1)
                fputc(' ', fp);
            else
                fputc('\n', fp);
        }

        from += freeze_linesize;
    }

    v_pop_indent();
    v_indent(fp);
    fputc('}', fp);

    v_freeze_finish(fp);

    return 1;
}

/* Set vector freeze format string */
void
vv_freeze_format(char *string)
{
    freeze_format = string;
}

/* Set no. of elements per vector freeze line */
void
vv_freeze_linesize(int size)
{
    freeze_linesize = size;
}

/*!
  @brief   Get element of a vector.
  @ingroup vector_access
  @param   v Vector.
  @param   num Index entry.
  @return  Value.
*/
float
vv_get(vvector *v, int num)
{
    VV_CHECK(v);

    if (num < 0 || num >= v->size) {
        v_exception("vv_get(): attempt to reference invalid element (%d)",
                    num);
        return 0.0;
    }

    return v->data[num];
}

/*!
  @brief   Return a pointer to the values in a vector.
  @ingroup vector_access
  @param   v Vector.
  @return  List of values (don't deallocate it).
*/
float *
vv_get_values(vvector *v)
{
    VV_CHECK(v);
    return v->data;
}

/*!
  @brief   Return a copy of the values in a vector.
  @ingroup vector_access
  @param   v Vector.
  @return  Copy of the values.
*/
float *
vv_get_values_copy(vvector *v)
{
    float *values;
    int i;

    VV_CHECK(v);

    values = V_ALLOC(float, v->size);
    for (i = 0; i < v->size; i++)
        values[i] = v->data[i];

    return values;
}

/*!
  @brief   Return length of a vector.
  @ingroup vector_misc
  @param   v Vector.
  @return  Length.
*/
int
vv_length(vvector *v)
{
    VV_CHECK(v);
    return v->size;
}

/* Print contents of a vector */
void
vv_print(vvector *v, FILE *fp)
{
    int from = 0, to, i;

    VV_CHECK(v);

    v_print_start();
    v_push_indent();

    v_print_type(vvector_type, v, fp);

    while (from < v->size) {
        to = V_MIN(from + print_linesize, v->size);
        v_indent(fp);

        for (i = from; i < to; i++) {
            fprintf(fp, print_format, v->data[i]);

            if (i < to - 1)
                fputc(' ', fp);
            else
                fputc('\n', fp);
        }

        from += print_linesize;
    }

    v_pop_indent();
    v_print_finish();
}

/* Set vector print format string */
void
vv_print_format(char *string)
{
    print_format = string;
}

/* Set no. of elements per vector print line */
void
vv_print_linesize(int size)
{
    print_linesize = size;
}

/* Read vector from a stream */
vvector *
vv_read(FILE *fp)
{
    vvector *v;
    int size;

    /* No. of elements */
    if (!v_read_long(&size, fp))
        return NULL;

    /* List of floats */
    v = vv_create_with_values((unsigned int) size, NULL);
    if (!v_read_float_list(v->data, (int) size, fp)) {
        vv_destroy(v);
        return NULL;
    }

    return v;
}

/*!
  @brief   Sort vector values in ascending order.
  @ingroup vector_misc
  @param   v Vector.
*/
void
vv_sort(vvector *v)
{
    if (v->size > 1) 
	qsort(v->data, v->size, sizeof(float), vv_sortpoints);
}

/* Point sorting function */
static int
vv_sortpoints(const void *v1, const void *v2)
{
    float *p1 = (float *) v1;
    float *p2 = (float *) v2;

    /* Sort on X values */
    return ((*p1) < (*p2)) ? -1 : 1;
}


/*!
  @brief   Set vector element to a given value.
  @ingroup vector_modify
  @param   v Vector
  @param   num Index entry.
  @param   val Value to set.
*/
void
vv_store(vvector *v, int num, float val)
{
    VV_CHECK(v);

    if (num < 0 || num >= v->size) {
        v_exception("vv_store(): attempt to reference invalid element (%d)",
                    num);
        return;
    }

    v->data[num] = val;
}

/*!
  @brief   Set all vector elements to the given values.
  @ingroup vector_modify
  @param   v Vector.
  @param   values List of values to set.
*/
void
vv_store_values(vvector *v, float *values)
{
    int i;

    VV_CHECK(v);

    for (i = 0; i < v->size; i++)
        v->data[i] = (values == NULL ? 0.0 : values[i]);
}

/* Thaw vector from a stream */
vvector *
vv_thaw(FILE *fp)
{
    static int allocsize = ALLOCSIZE;
    static float *values = NULL;
    int token, count = 0;
    double value;

    v_thaw_start();

    if (!v_thaw_follow(fp, '{', "open-brace"))
        goto fail;

    if (values == NULL)
        values = V_ALLOC(float, allocsize);

    while (1) {
        if (v_thaw_peek(fp) == '}') {
            v_thaw_token(fp);
            break;
        }

        if (!v_thaw_double(fp, &value))
            goto fail;

        values[count] = value;

        if (++count == allocsize) {
            allocsize *= 2;
            values = V_REALLOC(values, float, allocsize);
        }

        if ((token = v_thaw_token(fp)) == '}') {
            break;
        } else if (token != ',') {
            v_thaw_expected("comma or close-brace");
            goto fail;
        }
    }

    v_thaw_finish();

    if (count == 0) {
        v_thaw_err("vector has no elements");
        goto fail;
    }

    return vv_create_from_values(count, values);

  fail:
    v_thaw_finish();
    return NULL;
}

/* Traverse a vector */
int
vv_traverse(vvector *v, int (*func)())
{
    VV_CHECK(v);

    if ((*func)(v))
        return 1;

    v_push_traverse(v);
    v_pop_traverse();

    return 0;
}

/* Write vector to a stream */
int
vv_write(vvector *v, FILE *fp)
{
    VV_CHECK(v);

    /* No. of elements */
    if (!v_write_long(v->size, fp))
        return 0;

    /* List of floats */
    if (!v_write_float_list(v->data, v->size, fp))
        return 0;

    return 1;
}

/* Dump contents of a vector in XML format */
static int
vv_xmldump(vvector *v, FILE *fp)
{
    char buf[30];
    int i;

    VV_CHECK(v);

    v_xmldump_start(fp);

    for (i = 0; i < v->size; i++) {
        sprintf(buf, freeze_format, v->data[i]);
        v_xmldump_tag(fp, "entry", "value", buf, NULL);
    }

    v_xmldump_finish(fp);
    return 1;
}
