/*!
  @file
  @brief YAML I/O functions.
*/

#ifndef VARS_YAML_H
#define VARS_YAML_H

#ifdef __cplusplus
extern "C" {
#endif

extern void *v_yaml_read(FILE *fp);
extern void *v_yaml_read_file(char *file);
extern int v_yaml_write(void *ptr, FILE *fp);
extern int v_yaml_write_file(void *ptr, char *file);
extern char *v_yaml_error(void);

#ifdef __cplusplus
}
#endif

#endif
