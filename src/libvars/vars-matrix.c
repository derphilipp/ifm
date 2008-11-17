/*!
  @defgroup matrix Matrices
  @ingroup types

  A matrix is a simple two-dimensional array of floats.  Matrices grow
  automatically to accommodate new entries.
*/

/*!
  @defgroup matrix_create Creating and destroying matrices
  @ingroup matrix
*/

/*!
  @defgroup matrix_modify Modifying matrices
  @ingroup matrix
*/

/*!
  @defgroup matrix_access Accessing matrix information
  @ingroup matrix
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vars-config.h"
#include "vars-macros.h"
#include "vars-matrix.h"
#include "vars-memory.h"

#define ALLOC_SIZE 32

/* Type definition */
struct v_matrix {
    struct v_header id;         /* Type marker */
    float **data;               /* Values */
    int nrows, ncols;           /* Size */
    int maxrows, maxcols;       /* Allocated size */
};

/* Type variable */
vtype *vmatrix_type = NULL;

/* Internal functions */
static int vm_xmldump(vmatrix *m, FILE *fp);

/* Check if pointer is a matrix */
int
vm_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vmatrix_type);
}

/*!
  @brief   Return the no. of columns in a matrix.
  @ingroup matrix_access
  @param   m Matrix.
  @return  No. of columns.
*/
int
vm_columns(vmatrix *m)
{
    VM_CHECK(m);
    return m->ncols;
}

/*!
  @brief   Return a copy of a matrix.
  @ingroup matrix_create
  @param   m Matrix.
  @return  Copy.
*/
vmatrix *
vm_copy(vmatrix *m)
{
    vmatrix *copy;
    int i, j;

    VM_CHECK(m);

    copy = vm_create_size(m->nrows, m->ncols);

    for (i = 0; i < m->nrows; i++)
        for (j = 0; j < m->ncols; j++)
            copy->data[i][j] = m->data[i][j];

    return copy;
}

/*!
  @brief   Return a newly-created matrix.
  @ingroup matrix_create
  @return  New matrix.
*/
vmatrix *
vm_create(void)
{
    return vm_create_size(0, 0);
}

/*!
  @brief   Return a newly-created matrix with a given size.
  @ingroup matrix_create
  @param   nrows No. of rows.
  @param   ncols No. of columns.
  @return  New matrix.
*/
vmatrix *
vm_create_size(int nrows, int ncols)
{
    static vheader *id = NULL;
    vmatrix *m;

    if (id == NULL) {
        vm_declare();
        id = v_header(vmatrix_type);
    }

    m = V_ALLOC(vmatrix, 1);
    m->id = *id;

    m->maxrows = m->nrows = 0;
    m->maxcols = m->ncols = 0;
    m->data = NULL;

    nrows = V_MAX(nrows, 1);
    ncols = V_MAX(ncols, 1);
    vm_set_size(m, nrows, ncols);

    return m;
}

/* Declare matrix type */
vtype *
vm_declare(void)
{
    if (vmatrix_type == NULL) {
        vmatrix_type = v_create("MATRIX", "M");
	v_create_func(vmatrix_type, (void *(*)()) vm_create);
        v_copy_func(vmatrix_type, (void *(*)()) vm_copy);
        v_read_func(vmatrix_type, (void *(*)()) vm_read);
        v_write_func(vmatrix_type, vm_write);
        v_freeze_func(vmatrix_type, vm_freeze);
        v_thaw_func(vmatrix_type, (void *(*)()) vm_thaw);
        v_print_func(vmatrix_type, vm_print);
        v_destroy_func(vmatrix_type, vm_destroy);
        v_traverse_func(vmatrix_type, vm_traverse);
        v_xmldump_func(vmatrix_type, vm_xmldump);
    }

    return vmatrix_type;
}

/*!
  @brief   Deallocate a matrix.
  @ingroup matrix_create
  @param   m Matrix.
*/
void
vm_destroy(vmatrix *m)
{
    int i;

    VM_CHECK(m);

    if (m->data != NULL) {
        for (i = 0; i < m->maxrows; i++)
            V_DEALLOC(m->data[i]);
        V_DEALLOC(m->data);
    }

    V_DEALLOC(m);
}

/* Freeze matrix to a stream */
int
vm_freeze(vmatrix *m, FILE *fp)
{
    int i, j;

    VM_CHECK(m);

    v_freeze_start(fp);

    fprintf(fp, "SIZE %d %d {\n", m->nrows, m->ncols);
    v_push_indent();

    for (i = 0; i < m->nrows; i++) {
        v_indent(fp);
        for (j = 0; j < m->ncols; j++)
            fprintf(fp, "%g%s", m->data[i][j],
                    j < m->ncols - 1 ? " " : ",\n");
    }

    v_pop_indent();

    v_indent(fp);
    fputc('}', fp);

    v_freeze_finish(fp);

    return 1;
}

/*!
  @brief   Get a value from a matrix.
  @ingroup matrix_access
  @param   m Matrix.
  @param   row Row number.
  @param   col Column number.
  @return  Matrix value.
*/
float
vm_get(vmatrix *m, int row, int col)
{
    VM_CHECK(m);

    row = V_MAX(row, 0);
    col = V_MAX(col, 0);

    if (row <= m->nrows && col <= m->ncols)
        return m->data[row][col];

    return 0.0;
}

/* Print contents of a matrix */
void
vm_print(vmatrix *m, FILE *fp)
{
    int i, j;

    VM_CHECK(m);

    v_print_start();
    v_push_indent();

    v_print_type(vmatrix_type, m, fp);

    for (i = 0; i < m->nrows; i++) {
        v_indent(fp);
        for (j = 0; j < m->ncols; j++)
            fprintf(fp, "%8.2f%s", m->data[i][j],
                    j < m->ncols - 1 ? " " : "\n");
    }

    v_pop_indent();
    v_print_finish();
}

/* Read matrix from a stream */
vmatrix *
vm_read(FILE *fp)
{
    int nrows, ncols, i, j;
    vmatrix *m;

    /* Read matrix size */
    if (!v_read_long(&nrows, fp))
        return NULL;

    if (!v_read_long(&ncols, fp))
        return NULL;

    m = vm_create_size(nrows, ncols);

    /* Read elements */
    for (i = 0; i < m->nrows; i++)
        for (j = 0; j < m->ncols; j++)
            if (!v_read_float(&m->data[i][j], fp))
                return NULL;

    return m;
}

/* Increase a matrix allocation if required */
void
vm_require(vmatrix *m, int nrows, int ncols)
{
    int rows, cols, i, j;

    VM_CHECK(m);

    if (nrows > m->maxrows) {
        rows = V_MAX(ALLOC_SIZE, nrows - m->maxrows);

        if (m->data == NULL)
            m->data = V_ALLOC(float *, m->maxrows + rows);
        else
            m->data = V_REALLOC(m->data, float *, m->maxrows + rows);

        if (m->maxcols == 0)
            m->maxcols = ALLOC_SIZE;

        for (i = m->maxrows; i < m->maxrows + rows; i++) {
            m->data[i] = V_ALLOC(float, m->maxcols);
            for (j = 0; j < m->maxcols; j++)
                m->data[i][j] = 0.0;
        }

        m->maxrows += rows;
    }

    if (ncols > m->maxcols) {
        cols = V_MAX(ALLOC_SIZE, ncols - m->maxcols);

        for (i = 0; i < m->maxrows; i++) {
            m->data[i] = V_REALLOC(m->data[i], float, m->maxcols + cols);
            for (j = m->maxcols; j < m->maxcols + cols; j++)
                m->data[i][j] = 0.0;
        }

        m->maxcols += cols;
    }
}

/*!
  @brief   Return the no. of rows in a matrix.
  @ingroup matrix_access
  @param   m Matrix.
  @return  No. of rows.
*/
int
vm_rows(vmatrix *m)
{
    VM_CHECK(m);
    return m->nrows;
}

/*!
  @brief   Set the size of a matrix.
  @ingroup matrix_modify
  @param   m Matrix.
  @param   nrows No. of rows.
  @param   ncols No. of columns.

  Set the size of a matrix.  This can make a matrix smaller, but note that
  no memory is reclaimed.  Only destruction of the whole matrix does that.
*/
void
vm_set_size(vmatrix *m, int nrows, int ncols)
{
    VM_CHECK(m);
    vm_require(m, nrows, ncols);
    m->nrows = nrows;
    m->ncols = ncols;
}

/*!
  @brief   Store a value in a matrix.
  @ingroup matrix_modify
  @param   m Matrix.
  @param   row Row number.
  @param   col Column number.
  @param   val Value to set.
*/
void
vm_store(vmatrix *m, int row, int col, float val)
{
    VM_CHECK(m);

    row = V_MAX(row, 0);
    col = V_MAX(col, 0);

    if (row >= m->nrows || col >= m->ncols) {
        vm_require(m, row + 1, col + 1);
        m->data[row][col] = val;
        m->nrows = V_MAX(m->nrows, row + 1);
        m->ncols = V_MAX(m->ncols, col + 1);
    } else {
        m->data[row][col] = val;
    }
}

/* Thaw matrix from a stream */
vmatrix *
vm_thaw(FILE *fp)
{
    int nrows = 0, ncols = 0, row = 0, col = 0, end = 0;
    vmatrix *m = NULL;

    v_thaw_start();

    if (v_thaw_peek(fp) == V_TOKEN_ID) {
        v_thaw_token(fp);
        if (!V_STREQ(v_thaw_svalue, "SIZE")) {
            v_thaw_expected("SIZE or open-brace");
            goto fail;
        }

        if (!v_thaw_int(fp, &nrows))
            goto fail;

        if (!v_thaw_int(fp, &ncols))
            goto fail;
    }

    m = vm_create_size(nrows, ncols);

    if (!v_thaw_follow(fp, '{', "open-brace"))
        goto fail;

    while (!end) {
        switch (v_thaw_token(fp)) {
        case V_TOKEN_INT:
        case V_TOKEN_REAL:
            vm_store(m, row, col, v_thaw_dvalue);
            col++;
            break;
        case ',':
            row++;
            col = 0;
            break;
        case '}':
            end = 1;
            break;
        default:
            v_thaw_expected("matrix value, comma or close-brace");
            goto fail;
        }
    }

    v_thaw_finish();
    return m;

  fail:
    v_thaw_finish();

    if (m != NULL)
        vm_destroy(m);

    return NULL;
}

/* Traverse a matrix */
int
vm_traverse(vmatrix *m, int (*func)())
{
    int val;

    VM_CHECK(m);

    if ((val = func(m)) != 0)
        return val;

    v_push_traverse(m);
    v_pop_traverse();

    return 0;
}

/* Write matrix to a stream */
int
vm_write(vmatrix *m, FILE *fp)
{
    int i, j;

    VM_CHECK(m);

    /* Matrix size */
    if (!v_write_long(m->nrows, fp))
        return 0;

    if (!v_write_long(m->ncols, fp))
        return 0;

    /* Elements */
    for (i = 0; i < m->nrows; i++)
        for (j = 0; j < m->ncols; j++)
            if (!v_write_float(m->data[i][j], fp))
                return 0;

    return 1;
}

/* Dump contents of a matrix in XML format */
static int
vm_xmldump(vmatrix *m, FILE *fp)
{
    char ibuf[30], jbuf[30], buf[30];
    float val;
    int i, j;

    VM_CHECK(m);

    v_xmldump_start(fp);

    for (i = 0; i < m->nrows; i++) {
        for (j = 0; j < m->ncols; j++) {
            if ((val = m->data[i][j]) != 0.0) {
                sprintf(ibuf, "%d", i);
                sprintf(jbuf, "%d", j);
                sprintf(buf, "%.10g", val);
                v_xmldump_tag(fp, "entry", "i", ibuf, "j", jbuf,
                              "value", buf, NULL);
            }
        }
    }

    v_xmldump_finish(fp);
    return 1;
}
