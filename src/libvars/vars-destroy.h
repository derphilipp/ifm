/*!
  @file
  @brief Destruction functions.
*/

#ifndef VARS_DESTROY_H
#define VARS_DESTROY_H

#ifdef __cplusplus
extern "C" {
#endif

extern void v_destroy(void *ptr);
extern void v_destroy_default(void (*func)(void *ptr));
extern void v_destroy_with(void *ptr, void (*func)(void *ptr));

#ifdef __cplusplus
}
#endif

#endif
