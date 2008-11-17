/*!
  @file
  @brief Copying functions.
*/

#ifndef VARS_COPY_H
#define VARS_COPY_H

extern int v_deepcopy;

#ifdef __cplusplus
extern "C" {
#endif

extern void *v_copy(void *ptr);
extern void v_copy_default(void *(*func)(void *ptr));
extern void v_copy_with(void *ptr, void *(*func)(void *ptr));

#ifdef __cplusplus
}
#endif

#endif
