/*!
  @file
  @brief Regexp functions and macros.
  @ingroup regexp
*/

#ifndef VARS_REGEXP_H
#define VARS_REGEXP_H

#include <vars-list.h>

/*! @brief Abort if a pointer is not a regexp. */
#define VR_CHECK(ptr)           V_CHECK(ptr, vr_check, "REGEX")

/*! @brief Like vr_matched(), but copy the result. */
#define vr_matchedcopy(num) \
        V_STRDUP(vr_matched(num))

/*! @brief Like vr_subst(), but copy the result. */
#define vr_substcopy(str, pat, sub) \
        V_STRDUP(vr_subst(str, pat, sub))

/*! @brief Create a regexp if not already done. */
#define vr_init(r, regexp) \
        !(r == NULL && (r = vr_create(regexp)) == NULL)

/*! @brief Regexp type. */
typedef struct v_regex vregex;

#ifdef __cplusplus
extern "C" {
#endif

extern int vr_check(void *ptr);
extern vregex *vr_copy(vregex *r);
extern vregex *vr_create(char *str);
extern vregex *vr_create_nocase(char *str);
extern vregex *vr_create_translate(char *str, char *table);
extern vtype *vr_declare(void);
extern void vr_destroy(vregex *r);
extern char *vr_error(void);
extern vlist *vr_extract(char *string, vregex *r);
extern int vr_freeze(vregex *r, FILE *fp);
extern vlist *vr_grep(vlist *l, vregex *r);
extern int vr_match(char *str, vregex *r);
extern char *vr_matched(int num);
extern void vr_print(vregex *r, FILE *fp);
extern vregex *vr_read(FILE *fp);
extern char *vr_regexp(vregex *r);
extern char *vr_subst(char *str, vregex *r, char *sub);
extern vregex *vr_thaw(FILE *fp);
extern int vr_traverse(vregex *r, int (*func)(void *ptr));
extern int vr_write(vregex *r, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
