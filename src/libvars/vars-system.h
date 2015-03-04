/*!
  @file
  @brief System functions and macros.
  @ingroup system
*/

#ifndef VARS_SYSTEM_H
#define VARS_SYSTEM_H

#include <vars-list.h>
#include <vars-hash.h>

/*! @brief File tests. */
enum v_testtype {
    V_TEST_EXISTS,              /*!< Does it exist? */
    V_TEST_DIR,                 /*!< Is it a directory? */
    V_TEST_CHR,                 /*!< Is it character-special? */
    V_TEST_BLK,                 /*!< Is it block-special? */
    V_TEST_REG,                 /*!< Is it a regular file? */
    V_TEST_FIFO,                /*!< Is it a FIFO? */
};

/*! @brief Synonym for V_TEST_REG. */
#define V_TEST_FILE      V_TEST_REG

/*! @brief Test whether a file exists. */
#define v_exists(file)   v_test(file, V_TEST_EXISTS)

/*! @brief Test whether a file is a directory. */
#define v_isdir(file)    v_test(file, V_TEST_DIR)

/*! @brief Test whether a file is a symlink. */
#define v_islink(file)   v_test(file, V_TEST_LNK)

#ifdef __cplusplus
extern "C" {
#endif

extern char *v_dirname(char *path);
extern vhash *v_environ(void);
extern char *v_filename(char *path);
extern time_t v_parse_date(char *string);
extern vlist *v_stat(char *file);
extern char *v_suffix(char *path);
extern char *v_tempfile(char *prefix, char *buffer);
extern int v_test(char *file, enum v_testtype test);

#ifdef __cplusplus
}
#endif

#endif
