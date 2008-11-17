/* Compatibility with previous Vars versions */

#ifndef VARS_COMPAT_H
#define VARS_COMPAT_H

#include <vars.h>

#define v_isarray(ptr)    (ptr != NULL && v_type(ptr) == varray_type)
#define v_isbuffer(ptr)   (ptr != NULL && v_type(ptr) == vbuffer_type)
#define v_isdatabase(ptr) (ptr != NULL && v_type(ptr) == vdatabase_type)
#define v_isfunc(ptr)     (ptr != NULL && v_type(ptr) == vfunc_type)
#define v_isgraph(ptr)    (ptr != NULL && v_type(ptr) == vgraph_type)
#define v_ishash(ptr)     (ptr != NULL && v_type(ptr) == vhash_type)
#define v_isimage(ptr)    (ptr != NULL && v_type(ptr) == vimage_type)
#define v_islist(ptr)     (ptr != NULL && v_type(ptr) == vlist_type)
#define v_ismatrix(ptr)   (ptr != NULL && v_type(ptr) == vmatrix_type)
#define v_isparser(ptr)   (ptr != NULL && v_type(ptr) == vparser_type)
#define v_isqueue(ptr)    (ptr != NULL && v_type(ptr) == vqueue_type)
#define v_isregex(ptr)    (ptr != NULL && v_type(ptr) == vregex_type)
#define v_isscalar(ptr)   (ptr != NULL && v_type(ptr) == vscalar_type)
#define v_isunknown(ptr)  (ptr != NULL && v_type(ptr) == vunknown_type)
#define v_isvector(ptr)   (ptr != NULL && v_type(ptr) == vvector_type)

extern vtype *varray_type;
extern vtype *vbuffer_type;
extern vtype *vdatabase_type;
extern vtype *vfunc_type;
extern vtype *vgraph_type;
extern vtype *vhash_type;
extern vtype *vimage_type;
extern vtype *vlist_type;
extern vtype *vmatrix_type;
extern vtype *vparser_type;
extern vtype *vqueue_type;
extern vtype *vregex_type;
extern vtype *vscalar_type;
extern vtype *vunknown_type;
extern vtype *vvector_type;

#endif
