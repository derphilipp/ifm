/*!
  @file
  @brief Image functions and macros.
  @ingroup image
*/

#ifndef VARS_IMAGE_H
#define VARS_IMAGE_H

#include <vars-type.h>

/*! @brief Abort if a pointer is not an image. */
#define VI_CHECK(ptr)           V_CHECK(ptr, vi_check, "IMAGE")

/*! @brief Like vi_read_file(), but always guess format from suffix */
#define vi_read_image(file)     vi_read_file(file, NULL)

/*! @brief Like vi_write_file(), but always guess format from suffix */
#define vi_write_image(im, file) vi_write_file(im, file, NULL)

/*! @brief Return index into grayscale image data. */
#define vi_gray_index(width, i, j) ((width * j) + i)

/*! @brief Return index into RGB image data. */
#define vi_rgb_index(width, i, j)  (3 * vi_gray_index(width, i, j))

/*! @brief Image type. */
typedef struct v_image vimage;

#ifdef __cplusplus
extern "C" {
#endif

extern int vi_check(void *ptr);
extern void vi_compress(vimage *im, int flag);
extern vimage *vi_copy(vimage *im);
extern vimage *vi_copy_region(vimage *im, int x, int y, int width,
                              int height);
extern vimage *vi_create(int width, int height);
extern vtype *vi_declare(void);
extern void vi_delete_gray(vimage *im);
extern void vi_delete_rgb(vimage *im);
extern void vi_destroy(vimage *im);
extern char *vi_error(void);
extern int vi_freeze(vimage *im, FILE *fp);
extern int vi_get_gray(vimage *im, int i, int j, unsigned char *val);
extern unsigned char *vi_get_graydata(vimage *im);
extern unsigned char *vi_get_grayrow(vimage *im, int row);
extern int vi_get_height(vimage *im);
extern int vi_get_rgb(vimage *im, int i, int j, unsigned char *r,
                      unsigned char *g, unsigned char *b);
extern unsigned char *vi_get_rgbdata(vimage *im);
extern unsigned char *vi_get_rgbrow(vimage *im, int row);
extern int vi_get_width(vimage *im);
extern int vi_has_gray(vimage *im);
extern int vi_has_rgb(vimage *im);
extern int vi_has_rgba(vimage *im);
extern void vi_need_gray(vimage *im);
extern void vi_need_rgb(vimage *im);
extern void vi_print(vimage *im, FILE *fp);
extern vimage *vi_read(FILE *fp);
extern vimage *vi_read_file(char *file, char *format);
extern vimage *vi_read_pgm(FILE *fp);
extern vimage *vi_read_ppm(FILE *fp);
extern vimage *vi_read_sgi(FILE *fp);
extern int vi_set_gray(vimage *im, int i, int j, unsigned char val);
extern void vi_set_graydata(vimage *im, unsigned char *data);
extern int vi_set_rgb(vimage *im, int i, int j, unsigned char r,
                      unsigned char g, unsigned char b);
extern void vi_set_rgbdata(vimage *im, unsigned char *data);
extern vimage *vi_thaw(FILE *fp);
extern int vi_traverse(vimage *im, int (*func)(void *ptr));
extern int vi_write(vimage *im, FILE *fp);
extern int vi_write_file(vimage *im, char *file, char *format);
extern int vi_write_pgm(vimage *im, FILE *fp);
extern int vi_write_ppm(vimage *im, FILE *fp);
extern int vi_write_sgi(vimage *im, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
