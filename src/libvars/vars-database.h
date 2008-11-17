/*!
  @file
  @brief Database functions and macros.
  @ingroup database
*/

#ifndef VARS_DATABASE_H
#define VARS_DATABASE_H

#include <vars-hash.h>

/*! @brief Abort if a pointer is not a database. */
#define VD_CHECK(ptr)       V_CHECK(ptr, vd_check, "DATABASE")

/*! @brief Iteration record value. */
#define vd_iter_rec(iter)   iter.pval[0]

/* Type abbreviation */
typedef struct v_database vdatabase;

/*! @brief Field data types. */
enum v_datatype {
    V_FIELD_NUMBER,             /*!< Numeric */
    V_FIELD_STRING,             /*!< String */
    V_FIELD_MONTH,              /*!< Month */
};

/*! @brief Field alignments. */
enum v_alignment {
    V_ALIGN_LEFT,               /*!< Left */
    V_ALIGN_RIGHT,              /*!< Right */
};

/*! @brief Database formats. */
enum v_dbformat {
    V_FORMAT_RDB,               /*!< RDB */
    V_FORMAT_CSV,               /*!< Comma-separated value */
};

#ifdef __cplusplus
extern "C" {
#endif

extern int vd_check(void *ptr);
extern vdatabase *vd_copy(vdatabase *d);
extern vdatabase *vd_create(void);
extern vtype *vd_declare(void);
extern void vd_destroy(vdatabase *d);
extern void vd_empty(vdatabase *d);
extern char *vd_error(void);
extern int vd_export(vdatabase *d, enum v_dbformat type, FILE *fp);
extern int vd_export_file(vdatabase *d, enum v_dbformat type, char *file);
extern int vd_field_add(vdatabase *d, char *tag, char *desc,
                        enum v_datatype type);
extern enum v_alignment vd_field_align(vdatabase *d, int i);
extern int vd_field_calculate(vdatabase *d, char *expr);
extern int vd_field_count(vdatabase *d);
extern int vd_field_delete(vdatabase *d, char *tag);
extern char *vd_field_desc(vdatabase *d, int i);
extern char *vd_field_format(vdatabase *d, int i);
extern int vd_field_index(vdatabase *d, char *tag);
extern int vd_field_remove(vdatabase *d, char *tag, ...);
extern int vd_field_remove_list(vdatabase *d, vlist *fields);
extern int vd_field_select(vdatabase *d, char *field, ...);
extern int vd_field_select_list(vdatabase *d, vlist *fields);
extern int vd_field_set_align(vdatabase *d, int i, enum v_alignment align);
extern int vd_field_set_desc(vdatabase *d, int i, char *desc);
extern int vd_field_set_format(vdatabase *d, int i, char *format);
extern int vd_field_set_tag(vdatabase *d, int i, char *tag);
extern int vd_field_set_type(vdatabase *d, int i, enum v_datatype type);
extern int vd_field_set_width(vdatabase *d, int i, int width);
extern char *vd_field_tag(vdatabase *d, int i);
extern enum v_datatype vd_field_type(vdatabase *d, int i);
extern int vd_field_width(vdatabase *d, int i);
extern int vd_field_valid(char *str);
extern int vd_freeze(vdatabase *d, FILE *fp);
extern vdatabase *vd_import(enum v_dbformat type, FILE *fp);
extern vdatabase *vd_import_file(enum v_dbformat type, char *file);
extern int vd_next(viter *iter);
extern void vd_print(vdatabase *d, FILE *fp);
extern vdatabase *vd_read(FILE *fp);
extern int vd_record_add(vdatabase *d, vhash *rec);
extern int vd_record_count(vdatabase *d);
extern int vd_record_select(vdatabase *d, char *expr);
extern int vd_record_sort(vdatabase *d, char *field, ...);
extern int vd_record_sort_list(vdatabase *d, vlist *fields, vlist *rflags);
extern vdatabase *vd_thaw(FILE *fp);
extern int vd_traverse(vdatabase *d, int (*func)());
extern int vd_write(vdatabase *d, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
