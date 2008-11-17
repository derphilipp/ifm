/* TODO:
 *
 * CSV functions.
 * Remove extra hash data (e.g. when selecting/removing fields).
 * Implement join function(s).
 * Freeze/thaw: add comments.  Store comments without #.
 */

/*!
  @defgroup database Databases
  @ingroup types

  A database is an object which has a number of data fields and a set of
  records of data.  Each field has several attributes: tag name,
  description, type, width, and alignment.  Databases can be read and
  written in various formats, and there are functions to select fields and
  records in a relational-database style.

  The database object type is modelled on the RDB database format designed
  by Walt Hobbs (\c hobbs@rand.org).  It's still under development.
*/

/*!
  @defgroup database_create Creating and destroying databases
  @ingroup database
*/

/*!
  @defgroup database_access Accessing database fields and records
  @ingroup database
*/

/*!
  @defgroup database_modify Modifying database fields and records
  @ingroup database
*/

/*!
  @defgroup database_dbopts Relational database operations
  @ingroup database
*/

/*!
  @defgroup database_io Importing and exporting databases
  @ingroup database
*/

/*!
  @defgroup database_misc Other database functions
  @ingroup database
*/

/*!
  @defgroup database_fieldfmt Database field formats
  @ingroup database

  The format of a database field can be specified compactly using a format
  string.  The vd_field_format() and vd_field_set_format() functions get
  and set the format string, respectively.

  A format string has three components:

  -# A field width.  This is a number indicating the maximum width of the
     field.
  -# Optional type and alignment specifiers.  These are characters which
     immediately follow the field width.  The valid characters are:
     - \c S, \c N, or \c M.  Data type: string, number or month.  Default
       is string.
     - \c < or \c >.  Align left or right.  Default is left for strings,
       right for anything else.
     .
  -# Optional description.  This is indicated by a space (not part of the
     description) followed by characters up to the end of the format
     string.
  .

  For example, the format

  @verbatim
  20S> Address
  @endverbatim

  would mean a string of width 20, right-aligned, with description
  'Address'.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "vars-config.h"
#include "vars-copy.h"
#include "vars-buffer.h"
#include "vars-database.h"
#include "vars-hash.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-parser.h"

#define EXISTS(d, tag) vh_exists(d->tagmap, tag)

/* Type definition */
struct v_database {
    struct v_header id;         /* Type marker */

    vlist *fields;              /* List of fields */
    vlist *records;             /* List of data records */
    vlist *comments;            /* List of comment lines */

    vhash *tagmap;              /* Map of tags to IDs */
};

/* Field definition */
typedef struct v_field {
    char *tag;                  /* Tag name */
    char *desc;                 /* Description */
    enum v_datatype type;       /* Data type */
    enum v_alignment align;     /* Alignment */
    int width;                  /* Width (chars) */
} vfield;

/* Month names (for sorting) */
static struct month_st {
    char *name;
    int code;
} months [] = {
    { "jan",  1 },
    { "feb",  2 },
    { "mar",  3 },
    { "apr",  4 },
    { "may",  5 },
    { "jun",  6 },
    { "jul",  7 },
    { "aug",  8 },
    { "sep",  9 },
    { "oct", 10 },
    { "nov", 11 },
    { "dec", 12 },
    { NULL,   0 }
};

/* Type variable */
vtype *vdatabase_type = NULL;

/* Internal sorting parameters */
static char **sortfields = NULL;
static int *sortflags = NULL;
static int *sorttype = NULL;
static int sortcount = 0;

/* Internal functions */
static void vd_copy_fields(vdatabase *from, vdatabase *to);
static int vd_export_csv(vdatabase *d, FILE *fp);
static int vd_export_rdb(vdatabase *d, FILE *fp);
static vfield *vd_field_copy(vfield *f);
static void vd_field_destroy(vfield *f);
static char *vd_field_fmt(vfield *f);
static vfield *vd_field_new(char *tag, char *desc, enum v_datatype type);
static vdatabase *vd_import_csv(FILE *fp);
static vdatabase *vd_import_rdb(FILE *fp);
static int vd_month_code(char *str);
static vdatabase *vd_new(void);
static vparser *vd_parser(void);
static int vd_record_sort_func(vscalar **s1, vscalar **s2);
static void vd_update_tagmap(vdatabase *d);

/* Check if pointer is a database */
int
vd_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vdatabase_type);
}

/*!
  @brief   Return a copy of a database.
  @ingroup database_create
  @param   d Database.
  @return  Copy of the database.
*/
vdatabase *
vd_copy(vdatabase *d)
{
    vdatabase *copy;

    VD_CHECK(d);

    copy = vd_new();
    vd_copy_fields(d, copy);
    copy->records = v_copy(d->records);
    copy->comments = v_copy(d->comments);
    vd_update_tagmap(copy);

    return copy;
}

/* Copy fields of one database into another */
static void
vd_copy_fields(vdatabase *from, vdatabase *to)
{
    vfield *f;
    viter i;

    to->fields = vl_create();

    v_iterate(from->fields, i) {
        f = vl_iter_pval(i);
        f = vd_field_copy(f);
        vl_ppush(to->fields, f);
    }

    vd_update_tagmap(to);
}

/*!
  @brief   Create a new (empty) database.
  @ingroup database_create
  @return  New database.
*/
vdatabase *
vd_create(void)
{
    vdatabase *d = vd_new();

    d->fields = vl_create();
    d->records = vl_create();
    d->comments = vl_create();

    return d;
}

/* Declare database type */
vtype *
vd_declare(void)
{
    if (vdatabase_type == NULL) {
        vdatabase_type = v_create("DATABASE", "D");
	v_create_func(vdatabase_type, (void *(*)()) vd_create);
        v_copy_func(vdatabase_type, (void *(*)()) vd_copy);
        v_next_func(vdatabase_type, vd_next);
        v_read_func(vdatabase_type, (void *(*)()) vd_read);
        v_write_func(vdatabase_type, vd_write);
        v_freeze_func(vdatabase_type, vd_freeze);
        v_thaw_func(vdatabase_type, (void *(*)()) vd_thaw);
        v_print_func(vdatabase_type, vd_print);
        v_destroy_func(vdatabase_type, vd_destroy);
        v_traverse_func(vdatabase_type, vd_traverse);
    }

    return vdatabase_type;
}

/*!
  @brief   Deallocate a database.
  @ingroup database_create
  @param   d Database.
*/
void
vd_destroy(vdatabase *d)
{
    viter iter;
    vfield *f;

    VD_CHECK(d);

    v_iterate(d->fields, iter) {
        f = vl_iter_pval(iter);
        vd_field_destroy(f);
    }

    vl_destroy(d->fields);
    v_destroy(d->records);
    vl_destroy(d->comments);
    vh_destroy(d->tagmap);

    V_DEALLOC(d);
}

/*!
  @brief   Remove all records from a database.
  @ingroup database_modify
  @param   d Database.
*/
void
vd_empty(vdatabase *d)
{
    VD_CHECK(d);

    v_destroy(d->records);
    d->records = vl_create();
}

/*!
  @brief   Return the last database expression error.
  @ingroup database_dbopts
  @return  Error string.
  @retval  NULL if no errors occurred.
*/
char *
vd_error(void)
{
    vparser *p = vd_parser();
    return vp_error(p);
}

/*!
  @brief   Export database to a stream.
  @ingroup database_io
  @param   d Database.
  @param   type Export type.
  @param   fp Stream to write to.
  @return  Whether successful.
*/
int
vd_export(vdatabase *d, enum v_dbformat type, FILE *fp)
{
    VD_CHECK(d);

    switch (type) {
    case V_FORMAT_RDB:
        return vd_export_rdb(d, fp);
    case V_FORMAT_CSV:
        return vd_export_csv(d, fp);
    }

    v_exception("invalid export format");
    return 0;
}

/* Export data in CSV format */
static int
vd_export_csv(vdatabase *d, FILE *fp)
{
    /* FINISH ME */
    v_fatal("CSV export not yet implemented");

    return 0;
}

/*!
  @brief   Export database to file.
  @ingroup database_io
  @param   d Database.
  @param   type Export type.
  @param   file File to write to.
  @return  Whether successful.
*/
int
vd_export_file(vdatabase *d, enum v_dbformat type, char *file)
{
    int ok = 0;
    FILE *fp;

    VD_CHECK(d);

    if ((fp = fopen(file, "w")) != NULL) {
        ok = vd_export(d, type, fp);
        fclose(fp);
    }

    return ok;
}

/* Export data in RDB format */
static int
vd_export_rdb(vdatabase *d, FILE *fp)
{
    int i, num;
    vhash *rec;
    viter iter;
    vfield *f;

    /* Write comments */
    v_iterate(d->comments, iter)
        fputs(vl_iter_svalref(iter), fp);

    /* Write fields */
    num = vl_length(d->fields);
    for (i = 0; i < num; i++) {
        f = vl_pget(d->fields, i);
        fputs(f->tag, fp);
        fputc(i < num - 1 ? '\t' : '\n', fp);
    }

    for (i = 0; i < num; i++) {
        f = vl_pget(d->fields, i);
        fputs(vd_field_fmt(f), fp);
        fputc(i < num - 1 ? '\t' : '\n', fp);
    }

    /* Write records */
    v_iterate(d->records, iter) {
        rec = vl_iter_pval(iter);
        for (i = 0; i < num; i++) {
            f = vl_pget(d->fields, i);
            fputs(vh_sget(rec, f->tag), fp);
            fputc(i < num - 1 ? '\t' : '\n', fp);
        }
    }

    return 1;
}

/*!
  @brief   Add a field to a database.
  @ingroup database_modify
  @param   d Database.
  @param   tag Field tag.
  @param   desc Field description (or \c NULL).
  @param   type Field type.
  @return  Field index.
  @retval  -1 if field already exists, or tag is invalid.

  If a field description is not given, it defaults to the empty string.
*/
int
vd_field_add(vdatabase *d, char *tag, char *desc, enum v_datatype type)
{
    int idx = -1;
    vfield *f;

    VD_CHECK(d);

    if (!vd_field_valid(tag))
        return -1;

    if (!EXISTS(d, tag)) {
        f = vd_field_new(tag, desc, type);
        idx = vl_ppush(d->fields, f) - 1;
        vd_update_tagmap(d);
    }

    return idx;
}

/*!
  @brief   Return alignment of a field.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Alignment.
*/
enum v_alignment
vd_field_align(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return f->align;

    return 0;
}

/*!
  @brief   Calculate field values.
  @ingroup database_dbopts
  @param   d Database.
  @param   expr Expression evaluated on each record.
  @return  No. of values changed.
  @retval  -1 if expression is invalid.
  @see     vd_error()

  The expression should set one or more field values using the tag names of
  existing fields.  The variable \c recnum is also available as the number
  of the current record (counting from 1).
*/
int
vd_field_calculate(vdatabase *d, char *expr)
{
    vscalar *oldval, *newval, *val;
    int changed = 0, recnum = 0;
    vparser *p = vd_parser();
    vhash *rec;
    viter i, j;
    char *var;
    vfield *f;

    VD_CHECK(d);

    /* Loop over records */
    v_iterate(d, i) {
        rec = vd_iter_rec(i);

        /* Set current variable values */
        v_iterate(rec, j) {
            var = vh_iter_key(j);
            val = vh_iter_val(j);
            vp_store(p, var, vs_copy(val));
        }

        /* Set current record no. */
        recnum++;
        vp_istore(p, "recnum", recnum);

        /* Evaluate expression */
        if (!vp_eval(p, expr))
            return -1;

        /* Set field values */
        v_iterate(d->fields, j) {
            f = vl_iter_pval(j);
            oldval = vh_get(rec, f->tag);
            newval = vp_get(p, f->tag);
            if (oldval == NULL || !vs_equal(oldval, newval)) {
                vh_store(rec, f->tag, vs_copy(newval));
                changed++;
            }
        }
    }

    return changed;
}

/* Copy a field */
static vfield *
vd_field_copy(vfield *f)
{
    vfield *copy = V_ALLOC(vfield, 1);

    copy->tag = V_STRDUP(f->tag);
    copy->desc = V_STRDUP(f->desc);
    copy->type = f->type;
    copy->align = f->align;
    copy->width = f->width;

    return copy;
}

/*!
  @brief   Return no. of fields in a database.
  @ingroup database_access
  @param   d Database.
  @return  No. of fields.
*/
int
vd_field_count(vdatabase *d)
{
    VD_CHECK(d);
    return vl_length(d->fields);
}

/*!
  @brief   Delete a field.
  @ingroup database_dbopts
  @param   d Database.
  @param   tag Name of field to delete.
  @return  Whether successful.
*/
int
vd_field_delete(vdatabase *d, char *tag)
{
    vfield *f;
    int idx;

    VD_CHECK(d);

    if ((idx = vd_field_index(d, tag)) < 0)
        return 0;

    f = vl_premove(d->fields, idx);
    vd_field_destroy(f);
    vd_update_tagmap(d);

    return 1;
}

/*!
  @brief   Return description of a field.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Description.
*/
char *
vd_field_desc(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return f->desc;

    return NULL;
}

/* Destroy a field */
static void
vd_field_destroy(vfield *f)
{
    V_DEALLOC(f->tag);
    V_DEALLOC(f->desc);
    V_DEALLOC(f);
}

/* Return format string of a field */
static char *
vd_field_fmt(vfield *f)
{
    int rjust = 0;
    V_BUF_DECL;

    V_BUF_SET1("%d", f->width);

    switch (f->type) {

    case V_FIELD_STRING:
        V_BUF_ADD("S");
        break;

    case V_FIELD_NUMBER:
        V_BUF_ADD("N");
        rjust = 1;
        break;

    case V_FIELD_MONTH:
        V_BUF_ADD("M");
        break;
    }

    switch (f->align) {

    case V_ALIGN_LEFT:
        if (rjust)
            V_BUF_ADD("<");
        break;

    case V_ALIGN_RIGHT:
        if (!rjust)
            V_BUF_ADD(">");
        break;
    }

    V_BUF_ADD1(" %s", f->desc);
    return V_BUF_VAL;
}

/*!
  @brief   Return format string of a field.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Format string (see \ref database_fieldfmt).
*/
char *
vd_field_format(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return vd_field_fmt(f);

    return NULL;
}

/*!
  @brief   Find the index of a field.
  @ingroup database_access
  @param   d Database.
  @param   tag Field tag.
  @return  Field index.
  @retval  -1 if the field doesn't exist.
*/
int
vd_field_index(vdatabase *d, char *tag)
{
    VD_CHECK(d);

    if (EXISTS(d, tag))
        return vh_iget(d->tagmap, tag);

    return -1;
}

/* Create a new field */
static vfield *
vd_field_new(char *tag, char *desc, enum v_datatype type)
{
    vfield *f = V_ALLOC(vfield, 1);

    f->tag = V_STRDUP(tag);
    f->desc = V_STRDUP(desc ? desc : "");
    f->type = type;
    f->align = (type == V_FIELD_NUMBER ? V_ALIGN_RIGHT : V_ALIGN_LEFT);
    f->width = (type == V_FIELD_NUMBER ? 5 : 10);

    return f;
}

/*!
  @brief   Remove fields by name.
  @ingroup database_dbopts
  @param   d Database.
  @param   tag First field in variable arguments list.
  @return  New no. of fields.

  The list of fields must be terminated by \c NULL, or chaos will surely
  result.
*/
int
vd_field_remove(vdatabase *d, char *tag, ...)
{
    va_list ap;

    VD_CHECK(d);

    va_start(ap, tag);

    while (tag != NULL) {
        vd_field_delete(d, tag);
        tag = va_arg(ap, char *);
    }

    va_end(ap);

    return vl_length(d->fields);
}

/*!
  @brief   Remove fields by list of names.
  @ingroup database_dbopts
  @param   d Database.
  @param   fields List of field names.
  @return  New no. of fields.
*/
int
vd_field_remove_list(vdatabase *d, vlist *fields)
{
    viter iter;
    char *tag;

    VD_CHECK(d);
    VL_CHECK(fields);

    v_iterate(fields, iter) {
        tag = vl_iter_svalref(iter);
        vd_field_delete(d, tag);
    }

    return vl_length(d->fields);
}

/*!
  @brief   Select fields by name.
  @ingroup database_dbopts
  @param   d Database.
  @param   field First field in variable arguments list.
  @return  New no. of fields.
  @see     vd_field_select_list()

  The list of fields must be terminated by \c NULL, or chaos will surely
  result.
*/
int
vd_field_select(vdatabase *d, char *field, ...)
{
    vlist *fields;
    va_list ap;
    int ok;

    VD_CHECK(d);

    /* Build field list from arguments */
    fields = vl_create();
    va_start(ap, field);

    while (field != NULL) {
        vl_spush(fields, field);
        field = va_arg(ap, char *);
    }

    va_end(ap);

    /* Do the selection */
    ok = vd_field_select_list(d, fields);

    /* Clean up */
    vl_destroy(fields);

    return ok;
}

/*!
  @brief   Select fields by list of names.
  @ingroup database_dbopts
  @param   d Database.
  @param   fields List of field names.
  @return  New no. of fields.
*/
int
vd_field_select_list(vdatabase *d, vlist *fields)
{
    int idx, num = 0;
    vhash *seen;
    vlist *list;
    viter iter;
    char *tag;
    vfield *f;

    VD_CHECK(d);

    /* Build new field list */
    list = vl_create();
    seen = vh_create();

    v_iterate(fields, iter) {
        tag = vl_iter_svalref(iter);
        if ((idx = vd_field_index(d, tag)) >= 0) {
            f = vl_pget(d->fields, idx);
            if (!vh_exists(seen, f->tag)) {
                vl_ppush(list, vd_field_copy(f));
                vh_undef(seen, f->tag);
                num++;
            }
        }
    }

    /* Dispose of old fields */
    v_iterate(d->fields, iter) {
        f = vl_iter_pval(iter);
        vd_field_destroy(f);
    }

    /* Replace list */
    vl_destroy(d->fields);
    d->fields = list;
    vd_update_tagmap(d);

    return num;
}

/*!
  @brief   Set field alignment.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   align Alignment.
  @return  Whether successful.
*/
int
vd_field_set_align(vdatabase *d, int i, enum v_alignment align)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL) {
        f->align = align;
        return 1;
    }

    return 0;
}

/*!
  @brief   Set field description.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   desc Description (or \c NULL).
  @return  Whether successful.
*/
int
vd_field_set_desc(vdatabase *d, int i, char *desc)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL) {
        V_DEALLOC(f->desc);
        f->desc = V_STRDUP(desc ? desc : "");
        return 1;
    }

    return 0;
}

/*!
  @brief   Set field format.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   format Field format string (see \ref database_fieldfmt).
  @return  Whether successful.
*/
int
vd_field_set_format(vdatabase *d, int i, char *format)
{
    enum v_alignment align = V_ALIGN_RIGHT;
    int white = 0, width, alignset = 0;
    char *cp = format;

    VD_CHECK(d);

    if (vl_pget(d->fields, i) == NULL)
        return 0;

    if ((width = atoi(format)) > 0)
        vd_field_set_width(d, i, width);

    while (*cp != '\0') {
        if (isspace(*cp)) {
            white = 1;
        } else if (white && !isspace(*cp)) {
            vd_field_set_desc(d, i, cp);
            break;
        } else if (*cp == 'S') {
            vd_field_set_type(d, i, V_FIELD_STRING);
            align = V_ALIGN_LEFT;
        } else if (*cp == 'N') {
            vd_field_set_type(d, i, V_FIELD_NUMBER);
        } else if (*cp == 'M') {
            vd_field_set_type(d, i, V_FIELD_MONTH);
        } else if (*cp == '<') {
            vd_field_set_align(d, i, V_ALIGN_LEFT);
            alignset = 1;
        } else if (*cp == '>') {
            vd_field_set_align(d, i, V_ALIGN_RIGHT);
            alignset = 1;
        }

        cp++;
    }

    if (!alignset)
        vd_field_set_align(d, i, align);

    return 1;
}

/*!
  @brief   Set field tag.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   tag New field tag.
  @return  Whether successful.
*/
int
vd_field_set_tag(vdatabase *d, int i, char *tag)
{
    vscalar *val;
    vhash *data;
    viter iter;
    vfield *f;
    int idx;

    VD_CHECK(d);

    /* Get old field tag */
    idx = vd_field_index(d, tag);
    if (idx != i && idx >= 0)
        return 0;

    if ((f = vl_pget(d->fields, i)) == NULL)
        return 0;

    /* Update tags on all records */
    v_iterate(d->records, iter) {
        data = vl_iter_pval(iter);
        val = vh_get(data, f->tag);
        vh_store(data, tag, vs_copy(val));
        vh_delete(data, f->tag);
    }

    /* Set new field tag */
    V_DEALLOC(f->tag);
    f->tag = V_STRDUP(tag);

    return 1;
}

/*!
  @brief   Set field type.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   type Field type.
  @return  Whether successful.
*/
int
vd_field_set_type(vdatabase *d, int i, enum v_datatype type)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL) {
        f->type = type;
        return 1;
    }

    return 0;
}

/*!
  @brief   Set field width.
  @ingroup database_modify
  @param   d Database.
  @param   i Field index.
  @param   width Field width.
  @return  Whether successful.
*/
int
vd_field_set_width(vdatabase *d, int i, int width)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL) {
        f->width = width;
        return 1;
    }

    return 0;
}

/*!
  @brief   Return a field tag.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Tag string.
  @retval  NULL if field doesn't exist.
*/
char *
vd_field_tag(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return f->tag;

    return NULL;
}

/*!
  @brief   Return a field type.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Width.
  @retval  0 if field doesn't exist.
*/
enum v_datatype
vd_field_type(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return f->type;

    return 0;
}

/*!
  @brief   Return a field width.
  @ingroup database_access
  @param   d Database.
  @param   i Field index.
  @return  Width.
  @retval  0 if field doesn't exist.
*/
int
vd_field_width(vdatabase *d, int i)
{
    vfield *f;

    VD_CHECK(d);

    if ((f = vl_pget(d->fields, i)) != NULL)
        return f->width;

    return 0;
}

/*!
  @brief   Check string for validity as a field tag.
  @ingroup database_misc
  @param   str String.
  @return  Whether valid as a tag.
*/
int
vd_field_valid(char *str)
{
    if (strpbrk(str, " \t\f\n\r") != NULL)
        return 0;

    return 1;
}

/* Freeze database to a stream */
int
vd_freeze(vdatabase *d, FILE *fp)
{
    vhash *data;
    viter iter;
    vfield *f;

    VD_CHECK(d);

    v_freeze_start(fp);

    /* Write fields */
    fputs("{\n", fp);
    v_push_indent();

    v_indent(fp);
    fputs("FIELDS {\n", fp);
    v_push_indent();

    v_iterate(d->fields, iter) {
        f = vl_iter_pval(iter);
        v_indent(fp);

        fputs(f->tag, fp);
        fputs(" = ", fp);

        if (!v_freeze_string(vd_field_fmt(f), fp))
            return 0;

        fputs(",\n", fp);
    }

    v_pop_indent();
    v_indent(fp);
    fputs("},\n", fp);

    /* Write records */
    v_iterate(d->records, iter) {
        data = vl_iter_pval(iter);

        v_indent(fp);
        fputs("RECORD {\n", fp);
        v_push_indent();

        v_iterate(d->fields, iter) {
            f = vl_iter_pval(iter);
            v_indent(fp);

            fputs(f->tag, fp);
            fputs(" = ", fp);

            if (!vs_freeze(vh_get(data, f->tag), fp))
                return 0;

            fputs(",\n", fp);
        }

        v_pop_indent();
        v_indent(fp);
        fputs("},\n", fp);
    }

    v_pop_indent();
    v_indent(fp);
    fputc('}', fp);

    v_freeze_finish(fp);

    return 1;
}

/*!
  @brief   Import data to a database.
  @ingroup database_io
  @param   type Database type.
  @param   fp Stream to read from.
  @return  Database.
  @retval  NULL if the read failed.
*/
vdatabase *
vd_import(enum v_dbformat type, FILE *fp)
{
    switch (type) {
    case V_FORMAT_RDB:
        return vd_import_rdb(fp);
    case V_FORMAT_CSV:
        return vd_import_csv(fp);
    }
}

/* Import CSV data from a stream */
static vdatabase *
vd_import_csv(FILE *fp)
{
#if 0
    vdatabase *d;
    vhash *rec;
#endif

    /* FINISH ME */
    v_fatal("CSV import not yet implemented");

    return NULL;
}

/*!
  @brief   Import file to a database.
  @ingroup database_io
  @param   type Database type.
  @param   file File to read from.
  @return  Database.
  @retval  NULL if the read failed.
*/
vdatabase *
vd_import_file(enum v_dbformat type, char *file)
{
    vdatabase *d = NULL;
    FILE *fp;

    if ((fp = fopen(file, "r")) != NULL) {
        d = vd_import(type, fp);
        fclose(fp);
    }

    return d;
}

/* Import RDB data from a stream */
static vdatabase *
vd_import_rdb(FILE *fp)
{
    static vhash *rec = NULL;
    int i, lnum = 0, num;
    char *line, *string;
    vdatabase *d;
    vlist *data;
    V_BUF_DECL;
    vfield *f;

    d = vd_create();
    vh_init(rec);

    while ((line = V_BUF_FGETS(fp)) != NULL) {
        /* Deal with comment lines at start */
        if (lnum == 0 && line[0] == '#') {
            vl_spush(d->comments, line);
            continue;
        }

        lnum++;
        data = vl_nsplit(line, "\t\n");

        if (lnum == 1)
            num = vl_length(data);

        for (i = 0; i < num; i++) {
            string = vl_sgetref(data, i);

            switch (lnum) {

            case 1:
                vd_field_add(d, string, NULL, V_FIELD_NUMBER);
                break;

            case 2:
                vd_field_set_format(d, i, string);
                break;

            default:
                f = vl_pget(d->fields, i);
                vh_sstore(rec, f->tag, string);
                break;
            }
        }

        if (lnum > 2)
            vd_record_add(d, rec);

        vl_destroy(data);
    }

    return d;
}

/* Return month code given a string (for sorting) */
static int
vd_month_code(char *str)
{
    static vhash *map = NULL;
    char tag[4];
    int i;

    /* Initialize month mapping if required */
    if (map == NULL) {
        map = vh_create();
        for (i = 0; months[i].name != NULL; i++)
            vh_istore(map, months[i].name, months[i].code);
    }

    /* Build index string */
    for (i = 0; i < 3 && *str != '\0'; i++)
        tag[i] = tolower(*str++);

    tag[i] = '\0';

    /* Look up code */
    return vh_iget(map, tag);
}

/* Return a newly-created empty database */
static vdatabase *
vd_new(void)
{
    static vheader *id = NULL;
    vdatabase *d;

    if (id == NULL) {
        vd_declare();
        id = v_header(vdatabase_type);
    }

    d = V_ALLOC(vdatabase, 1);
    d->id = *id;

    d->fields = NULL;
    d->records = NULL;
    d->comments = NULL;
    d->tagmap = vh_create();

    return d;
}

/*!
  @brief   Iterate over database records.
  @ingroup database_access
  @param   iter Iterator.
  @return  Whether to continue.
  @see     v_iter(), v_next(), v_iterate()

  Iterate over the records in a database.  Returns whether there are any
  more records.  The iterator must have been initialized via v_iter() with
  a database object.

  After this call, the iterator structure will contain data on the next
  database record, which you can access using vd_iter_rec().
*/
int
vd_next(viter *iter)
{
    vdatabase *d = iter->object;
    vhash *rec;

    VD_CHECK(d);

    if ((rec = vl_pget(d->records, iter->ipos)) != NULL) {
        iter->pval[0] = rec;
        iter->ipos++;
        return 1;
    }

    return 0;
}

/* Create database parser */
static vparser *
vd_parser(void)
{
    static vparser *p = NULL;

    if (p == NULL) {
        p = vp_create();
        vp_sconst(p, "null", "");
        vp_error_undef(p, 0);
    }

    return p;
}

/* Print contents of a database */
void
vd_print(vdatabase *d, FILE *fp)
{
    VD_CHECK(d);

    v_print_start();
    v_push_indent();

    v_print_type(vdatabase_type, d, fp);

    v_indent(fp);
    fprintf(fp, "FIELDS => %d\n", vl_length(d->fields));

    v_indent(fp);
    fprintf(fp, "RECORDS => %d\n", vl_length(d->records));

    v_pop_indent();
    v_print_finish();
}

/* Read database from a stream */
vdatabase *
vd_read(FILE *fp)
{
    int i, j, num, nfields, nrec, type, align;
    char *str, tag[100];
    vdatabase *d;
    vscalar *val;
    vhash *rec;
    vfield *f;

    d = vd_create();

    /* Read comments */
    if (!v_read_long(&num, fp))
        return NULL;

    for (i = 0; i < num; i++) {
        if ((str = v_read_string(fp)) != NULL)
            vl_spush(d->comments, str);
        else
            return NULL;
    }

    /* Read fields */
    if (!v_read_long(&nfields, fp))
        return NULL;

    for (i = 0; i < nfields; i++) {
        if ((str = v_read_string(fp)) != NULL)
            strcpy(tag, str);
        else
            return NULL;

        if ((str = v_read_string(fp)) == NULL)
            return NULL;

        if (!v_read_long(&type, fp))
            return NULL;

        if (!v_read_long(&align, fp))
            return NULL;

        f = vd_field_new(tag, str, type);
        f->align = align;
        vl_ppush(d->fields, f);
    }

    /* Read records */
    if (!v_read_long(&nrec, fp))
        return NULL;

    for (i = 0; i < nrec; i++) {
        rec = vh_create();
        vl_ppush(d->records, rec);

        for (j = 0; j < nfields; j++) {
            f = vl_pget(d->fields, j);
            if ((val = vs_read(fp)) != NULL)
                vh_sstore(rec, f->tag, str);
            else
                return NULL;
        }
    }

    vd_update_tagmap(d);
    return d;
}

/*!
  @brief   Add a record to a database.
  @ingroup database_modify
  @param   d Database.
  @param   rec Record to add.
  @return  No. of records now in the database.

  The new record is copied into the database.  Each pointer reference in
  the hash data is removed.
*/
int
vd_record_add(vdatabase *d, vhash *rec)
{
    vscalar *elt;
    viter iter;

    VD_CHECK(d);

    /* Copy the data */
    rec = vh_copy(rec);

    /* Remove any pointer references */
    v_iterate(rec, iter) {
        elt = vh_iter_val(iter);
        if (vs_type(elt) == V_TYPE_POINTER)
            vs_undef(elt);
    }

    /* Add the record */
    return vl_ppush(d->records, rec);
}

/*!
  @brief   Return no. of records in a database.
  @ingroup database_access
  @param   d Database.
  @return  No. of records.
*/
int
vd_record_count(vdatabase *d)
{
    VD_CHECK(d);
    return vl_length(d->records);
}

/*!
  @brief   Select records according to an expression.
  @ingroup database_dbopts
  @param   d Database.
  @param   expr Expression.
  @return  No. of records selected.
  @retval  -1 if expression is invalid.
  @see     vd_error()
*/
int
vd_record_select(vdatabase *d, char *expr)
{
    vparser *p = vd_parser();
    vlist *in, *out;
    vscalar *val;
    vhash *rec;
    viter i, j;
    char *var;

    VD_CHECK(d);

    /* Initialize */
    in = vl_create();
    out = vl_create();

    /* Loop over records */
    v_iterate(d, i) {
        rec = vd_iter_rec(i);

        /* Set current variable values */
        v_iterate(rec, j) {
            var = vh_iter_key(j);
            val = vh_iter_val(j);
            vp_store(p, var, vs_copy(val));
        }

        /* Evaluate expression */
        if (!vp_eval(p, expr)) {
            vl_destroy(in);
            vl_destroy(out);
            return -1;
        }

        /* Add record to appropriate list */
        if (vp_ivalue(p))
            vl_ppush(in, rec);
        else
            vl_ppush(out, rec);
    }

    /* Replace records */
    vl_destroy(d->records);
    d->records = in;
    v_destroy(out);

    return vl_length(d->records);
}

/*!
  @brief   Sort database records by field names.
  @ingroup database_dbopts
  @param   d Database.
  @param   field First field in list.
  @return  No. of fields sorted on.

  The arguments are a list of (field, flag) pairs: \c field is the name of
  the field to sort, and \c flag indicates whether to sort in reverse
  order.  The list must be terminated with \c NULL, or chaos will surely
  result.
*/
int
vd_record_sort(vdatabase *d, char *field, ...)
{
    static vlist *fields = NULL;
    static vlist *rflags = NULL;
    va_list ap;

    VD_CHECK(d);

    /* Build field and flag lists */
    vl_init(fields);
    vl_init(rflags);

    va_start(ap, field);

    while (field != NULL) {
        vl_spush(fields, field);
        vl_ipush(rflags, va_arg(ap, int));
        field = va_arg(ap, char *);
    }

    va_end(ap);

    /* Do the sort */
    return vd_record_sort_list(d, fields, rflags);
}

/*!
  @brief   Sort database records by list of field names.
  @ingroup database_dbopts

  @param   d Database.
  @param   fields List of field names to sort on.
  @param   rflags List of reverse-sort flags per field (or \c NULL).

  @return  No. of fields sorted on.
*/
int
vd_record_sort_list(vdatabase *d, vlist *fields, vlist *rflags)
{
    int i, idx, num, rflag, type;
    char *tag;

    VD_CHECK(d);

    /* Initialize */
    num = vl_length(fields);
    sortfields = V_ALLOC(char *, num);
    sortflags = V_ALLOC(int, num);
    sorttype = V_ALLOC(int, num);
    sortcount = 0;

    for (i = 0; i < num; i++) {
        tag = vl_sgetref(fields, i);
        if ((idx = vd_field_index(d, tag)) >= 0) {
            rflag = (rflags != NULL ? vl_iget(rflags, i) : 0);
            type = vd_field_type(d, idx);

            sortfields[sortcount] = tag;
            sortflags[sortcount] = rflag;
            sorttype[sortcount] = type;

            sortcount++;
        }
    }

    /* Sort */
    if (sortcount > 0)
        vl_sort(d->records, vd_record_sort_func);

    /* Clean up */
    V_DEALLOC(sortfields);
    V_DEALLOC(sortflags);
    V_DEALLOC(sorttype);

    return sortcount;
}

/* Record sort function */
static int
vd_record_sort_func(vscalar **s1, vscalar **s2)
{
    vhash *rec1 = vs_pget(*s1);
    vhash *rec2 = vs_pget(*s2);
    char *tag, *str1, *str2;
    vscalar *val1, *val2;
    int i, flag, m1, m2;
    double n1, n2;

    for (i = 0; i < sortcount; i++) {
        tag = sortfields[i];

        val1 = vh_get(rec1, tag);
        val2 = vh_get(rec2, tag);

        switch (sorttype[i]) {

        case V_FIELD_NUMBER:
            n1 = vs_dget(val1);
            n2 = vs_dget(val2);
            flag = V_CMP(n1, n2);
            break;

        case V_FIELD_STRING:
            str1 = vs_sgetref(val1);
            str2 = vs_sgetref(val2);
            flag = strcmp(str1, str2);
            break;

        case V_FIELD_MONTH:
            str1 = vs_sgetref(val1);
            str2 = vs_sgetref(val2);
            m1 = vd_month_code(str1);
            m2 = vd_month_code(str2);
            flag = V_CMP(m1, m2);
            break;

        default:
            flag = 0;
            break;
        }

        if (sortflags[i])
            flag = !flag;

        if (flag != 0)
            return flag;
    }

    return 0;
}

/* Thaw database from a stream */
vdatabase *
vd_thaw(FILE *fp)
{
    int token, idx, fflag, rflag;
    vdatabase *d;
    vhash *rec;
    vscalar *s;
    char *tag;

    v_thaw_start();

    d = vd_create();
    rec = vh_create();

    if (!v_thaw_follow(fp, '{', "open-brace"))
        goto fail;

    while (1) {
        if ((token = v_thaw_token(fp)) == '}')
            break;

        fflag = rflag = 0;
        if (token == V_TOKEN_ID) {
            if      (V_STREQ(v_thaw_svalue, "FIELDS"))
                fflag = 1;
            else if (V_STREQ(v_thaw_svalue, "RECORD"))
                rflag = 1;
        }

        if (!fflag && !rflag) {
            v_thaw_expected("FIELDS or RECORD");
            goto fail;
        }

        if (!v_thaw_follow(fp, '{', "open-brace"))
            goto fail;

        while (1) {
            if ((token = v_thaw_peek(fp)) == '}') {
                v_thaw_token(fp);
                break;
            }

            if (v_thaw_follow(fp, V_TOKEN_ID, "field tag"))
                tag = V_STRDUP(v_thaw_svalue);
            else
                goto fail;

            if (!v_thaw_follow(fp, '=', "'='")) {
                v_free(tag);
                goto fail;
            }

            if (fflag) {
                /* Field definition */
                idx = vd_field_add(d, tag, NULL, V_FIELD_NUMBER);
                if (v_thaw_follow(fp, V_TOKEN_STRING, "field format string"))
                    vd_field_set_format(d, idx, v_thaw_svalue);
                else
                    goto fail;
            } else {
                /* Database record entry */
                if ((s = vs_thaw(fp)) != NULL)
                    vh_store(rec, tag, s);
                else
                    goto fail;
            }

            v_free(tag);

            if ((token = v_thaw_token(fp)) == '}') {
                break;
            } else if (token != ',') {
                v_thaw_expected("comma or close-brace");
                goto fail;
            }
        }

        if (rflag)
            vd_record_add(d, rec);

        if ((token = v_thaw_token(fp)) == '}') {
            break;
        } else if (token != ',') {
            v_thaw_expected("comma or close-brace");
            goto fail;
        }
    }

    v_thaw_finish();
    vh_destroy(rec);

    return d;

  fail:
    v_thaw_finish();
    vh_destroy(rec);
    vd_destroy(d);

    return NULL;
}

/* Traverse a database */
int
vd_traverse(vdatabase *d, int (*func)())
{
    int val;

    VD_CHECK(d);

    if ((val = func(d)) != 0)
        return val;

    v_push_traverse(d);
    v_pop_traverse();

    return 0;
}

/* Update tag mapping */
static void
vd_update_tagmap(vdatabase *d)
{
    int i, len = vl_length(d->fields);
    vfield *f;

    vh_empty(d->tagmap);

    for (i = 0; i < len; i++) {
        f = vl_pget(d->fields, i);
        vh_istore(d->tagmap, f->tag, i);
    }
}

/* Write database to a stream */
int
vd_write(vdatabase *d, FILE *fp)
{
    int i, j, num, nfields, nrec;
    vscalar *val;
    vhash *rec;
    vfield *f;
    char *str;

    VD_CHECK(d);

    /* Write comments */
    num = vl_length(d->comments);
    if (!v_write_long(num, fp))
        return 0;

    for (i = 0; i < num; i++) {
        str = vl_sgetref(d->comments, i);
        if (!v_write_string(str, fp))
            return 0;
    }

    /* Write fields */
    nfields = vl_length(d->fields);
    if (!v_write_long(nfields, fp))
        return 0;

    for (i = 0; i < nfields; i++) {
        f = vl_pget(d->fields, i);

        if (!v_write_string(f->tag, fp))
            return 0;

        if (!v_write_string(f->desc, fp))
            return 0;

        if (!v_write_long(f->type, fp))
            return 0;

        if (!v_write_long(f->align, fp))
            return 0;
    }

    /* Write records */
    nrec = vl_length(d->records);
    if (!v_write_long(nrec, fp))
        return 0;

    for (i = 0; i < nrec; i++) {
        rec = vl_pget(d->records, i);

        for (j = 0; j < nfields; j++) {
            f = vl_pget(d->fields, j);
            val = vh_get(rec, f->tag);
            if (!vs_write(val, fp))
                return 0;
        }
    }

    return 1;
}
