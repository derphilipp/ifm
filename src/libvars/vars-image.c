/*!
  @defgroup image Images
  @ingroup types

  An image is an object which can contain RGB and/or grayscale data of a
  given width and height.  If an image contains both RGB and grayscale
  data, the grayscale data can be treated as an alpha channel.  Any image
  function that tries to access RGB or grayscale data will create that data
  if it doesn't already exist.
*/

/*!
  @defgroup image_create Creating and destroying images
  @ingroup image
*/

/*!
  @defgroup image_modify Modifying images
  @ingroup image
*/

/*!
  @defgroup image_access Accessing image information
  @ingroup image
*/

/*!
  @defgroup image_io Reading and writing images
  @ingroup image
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include "vars-config.h"
#include "vars-buffer.h"
#include "vars-image.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-system.h"

#define SGI_MAGIC 0x01DA

#define CONVERT "convert -compress None -depth 8"
#define SILENT  "2>/dev/null"

#define GV(im, i, j)     ((im)->width * (j) + (i))
#define RV(im, i, j)     (3 * GV(im, i, j))

#define VAL(im, i, j, k) (im)->rgb[RV(im, i, j) + (k)]

#define RVAL(im, i, j)   VAL(im, i, j, 0)
#define GVAL(im, i, j)   VAL(im, i, j, 1)
#define BVAL(im, i, j)   VAL(im, i, j, 2)

#define GET_RGB(im, i, j, r, g, b)                                      \
        { r = RVAL(im, i, j); g = GVAL(im, i, j); b = BVAL(im, i, j); }

#define SET_RGB(im, i, j, r, g, b)                                      \
        { RVAL(im, i, j) = r; GVAL(im, i, j) = g; BVAL(im, i, j) = b; }

#define GRAY(im, i, j)                                                  \
        (im)->gray[GV(im, i, j)]

#define VALID(im, i, j)                                                 \
        (i >= 0 && i < (im)->width && j >= 0 && j < (im)->height)

#define SAMESIZE(a, b)                                                  \
        ((a)->width == (b)->width && (a)->height == (b)->height)

#define FORMAT(str)                                                     \
        (format != NULL && V_STREQ(format, str))

#define IMPORT(image, file, func, rflag) do {                           \
    if ((fp = fopen(file, "rb")) != NULL) {                             \
        im = func(fp);                                                  \
        fclose(fp);                                                     \
    } else {                                                            \
        if (rflag)                                                      \
            vi_err("can't read '%s': %s", file, strerror(errno));       \
        else                                                            \
            vi_err("can't convert image data");                         \
        im = NULL;                                                      \
    }                                                                   \
} while (0)

/* Type definition */
struct v_image {
    struct v_header id;         /* Type marker */
    int width, height;          /* Image size */
    int compress;               /* Compression flag */
    unsigned char *rgb;         /* RGB data */
    unsigned char *gray;        /* Gray/alpha data */
};

/* SGI image header structure */
typedef struct {
    unsigned short magic;
    unsigned char storage, bpp;
    unsigned short dimension, width, height, channels;
    unsigned int minval, maxval;
} sgiheader;

/* Type variable */
vtype *vimage_type = NULL;

/* Error buffer */
static char *errbuf = NULL;

/* Internal functions */
static void vi_err(char *fmt, ...);
static int vi_expandrow(unsigned char *optr, unsigned char *iptr,
                        unsigned int maxsize);
static unsigned char vi_getbyte(FILE *fp);
static int vi_getlong(FILE *fp);
static unsigned short vi_getshort(FILE *fp);
static int vi_putbyte(FILE *fp, unsigned char val);
static int vi_putlong(FILE *fp, unsigned int val);
static int vi_putshort(FILE *fp, unsigned short val);
static void vi_skip_comments(FILE *fp);

/* Check if pointer is an image */
int
vi_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vimage_type);
}

/*!
  @brief   Set image compression flag.
  @ingroup image_io
  @param   im Image.
  @param   flag Compression flag.

  Set whether to use compression when writing the image via vi_write().
*/
void
vi_compress(vimage *im, int flag)
{
    im->compress = flag;
}

/*!
  @brief   Copy an image.
  @ingroup image_create
  @param   im Image.
  @return  Copy.
*/
vimage *
vi_copy(vimage *im)
{
    vimage *copy;

    VI_CHECK(im);

    copy = vi_create(im->width, im->height);
    copy->compress = im->compress;

    if (im->rgb != NULL) {
        int size = im->width * im->height * 3;
        copy->rgb = V_ALLOC(unsigned char, size);
        memcpy(copy->rgb, im->rgb, (size_t) size);
    }

    if (im->gray != NULL) {
        int size = im->width * im->height;
        copy->gray = V_ALLOC(unsigned char, size);
        memcpy(copy->gray, im->gray, (size_t) size);
    }

    return copy;
}

/*!
  @brief   Copy a region out of an image and return it.
  @ingroup image_modify
  @param   im Image.
  @param   x Bottom left X coordinate.
  @param   y Bottom left Y coordinate.
  @param   width Region width.
  @param   height Region height.
  @return  Image.
*/
vimage *
vi_copy_region(vimage *im, int x, int y, int width, int height)
{
    int i, j, gray, rgb;
    vimage *region;

    VI_CHECK(im);

    /* Create image region */
    rgb = vi_has_rgb(im);
    gray = vi_has_gray(im);

    region = vi_create(width, height);
    region->compress = im->compress;

    if (rgb)
        vi_need_rgb(region);

    if (gray)
        vi_need_gray(region);

    /* Set pixel values */
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            if (!VALID(im, x + i, y + j))
                continue;

            if (rgb) {
                RVAL(region, i, j) = RVAL(im, x + i, y + j);
                GVAL(region, i, j) = GVAL(im, x + i, y + j);
                BVAL(region, i, j) = BVAL(im, x + i, y + j);
            }

            if (gray)
                GRAY(region, i, j) = GRAY(im, x + i, y + j);
        }
    }

    return region;
}

/*!
  @brief   Create an image.
  @ingroup image_create
  @param   width Width.
  @param   height Height.
  @return  New image.

  @note Once an image is created, its dimensions can't change.
*/
vimage *
vi_create(int width, int height)
{
    static vheader *id = NULL;
    vimage *im;

    if (id == NULL) {
	vi_declare();
	id = v_header(vimage_type);
    }

    im = V_ALLOC(vimage, 1);
    im->id = *id;

    im->width  = V_MAX(width, 1);
    im->height = V_MAX(height, 1);

    im->rgb = NULL;
    im->gray = NULL;

    im->compress = 0;

    return im;
}

/* Declare image type */
vtype *
vi_declare(void)
{
    if (vimage_type == NULL) {
        vimage_type = v_create("IMAGE", "I");
        v_copy_func(vimage_type, (void *(*)()) vi_copy);
        v_read_func(vimage_type, (void *(*)()) vi_read);
        v_write_func(vimage_type, vi_write);
        v_freeze_func(vimage_type, vi_freeze);
        v_thaw_func(vimage_type, (void *(*)()) vi_thaw);
        v_print_func(vimage_type, vi_print);
        v_destroy_func(vimage_type, vi_destroy);
        v_traverse_func(vimage_type, vi_traverse);
    }

    return vimage_type;
}

/*!
  @brief   Delete gray data from an image.
  @ingroup image_modify
  @param   im Image.
*/
void
vi_delete_gray(vimage *im)
{
    VI_CHECK(im);
    V_DEALLOC(im->gray);
}

/*!
  @brief   Delete RGB data from an image.
  @ingroup image_modify
  @param   im Image.
*/
void
vi_delete_rgb(vimage *im)
{
    VI_CHECK(im);
    V_DEALLOC(im->rgb);
}

/*!
  @brief   Deallocate an image.
  @ingroup image_create
  @param   im Image.
*/
void 
vi_destroy(vimage *im)
{
    VI_CHECK(im);

    V_DEALLOC(im->rgb);
    V_DEALLOC(im->gray);
    V_DEALLOC(im);
}

/* Give an image error */
static void
vi_err(char *fmt, ...)
{
    V_BUF_DECL;
    V_BUF_FMT(fmt, errbuf);
}

/* Return the last image error */
char *
vi_error(void)
{
    return errbuf;
}

/* Expand an RLE-compressed row */
static int
vi_expandrow(unsigned char *optr, unsigned char *iptr, unsigned int maxsize)
{
    unsigned char pixel, count;
    int total = 0;

    while (1) {
	pixel = *iptr++;
        if ((count = (pixel & 0x7f)) == 0)
            return 1;

	if (pixel & 0x80) {
	    while (count-- && total++ < maxsize)
		*optr++ = *iptr++;
	} else {
	    pixel = *iptr++;
	    while (count-- && total++ < maxsize)
		*optr++ = pixel;
	}

        if (total == maxsize)
            return 1;
    }
}

/* Freeze an image to a stream */
int
vi_freeze(vimage *im, FILE *fp)
{
    unsigned char r, g, b;
    int i, j, count;

    VI_CHECK(im);

    v_freeze_start(fp);

    if      (vi_has_rgba(im))
        fprintf(fp, "RGBA ");
    else if (vi_has_rgb(im))
        fprintf(fp, "RGB ");
    else if (vi_has_gray(im))
        fprintf(fp, "GRAY ");
    else
        fprintf(fp, "NONE ");

    fprintf(fp, "%d %d", im->width, im->height);

    if (im->rgb != NULL || im->gray != NULL) {
        fprintf(fp, " {\n");
        v_push_indent();

        for (j = 0; j < im->height; j++) {
            v_indent(fp);
            count = 0;

            for (i = 0; i < im->width; i++) {
                if (im->rgb != NULL) {
                    if (count++ > 0)
                        fputc(' ', fp);
                    GET_RGB(im, i, j, r, g, b);
                    fprintf(fp, "%d %d %d", r, g, b);
                }

                if (im->gray != NULL) {
                    if (count++ > 0 || im->rgb != NULL)
                        fputc(' ', fp);
                    fprintf(fp, "%d", GRAY(im, i, j));
                }
            }

            fprintf(fp, ",\n");
        }

        v_pop_indent();
        v_indent(fp);
        fputc('}', fp);
    }

    v_freeze_finish(fp);
    return 1;
}

/*!
  @brief   Return gray value of a pixel.
  @ingroup image_access
  @param   im Image.
  @param   i X pixel.
  @param   j Y pixel.
  @param[out]   val Pointer to value.
  @return  Whether coordinate is valid.
*/
int
vi_get_gray(vimage *im, int i, int j, unsigned char *val)
{
    VI_CHECK(im);
    vi_need_gray(im);

    if (!VALID(im, i, j))
        return 0;

    if (val != NULL)
        *val = GRAY(im, i, j);

    return 1;
}

/*!
  @brief   Get grayscale image data.
  @ingroup image_access
  @param   im Image.
  @return  Gray data (don't deallocate).
  @note    You can use vi_gray_index() to access pixel values.
*/
unsigned char *
vi_get_graydata(vimage *im)
{
    VI_CHECK(im);
    vi_need_gray(im);
    return im->gray;
}

/*!
  @brief   Get a row of grayscale image data.
  @ingroup image_access
  @param   im Image.
  @param   row Row number.
  @return  Image row (don't deallocate).
*/
unsigned char *
vi_get_grayrow(vimage *im, int row)
{
    VI_CHECK(im);
    vi_need_gray(im);
    return im->gray + GV(im, 0, row);
}

/*!
  @brief   Return image height.
  @ingroup image_access
  @param   im Image.
  @return  Height.
*/
int
vi_get_height(vimage *im)
{
    VI_CHECK(im);
    return im->height;
}

/*!
  @brief   Return RGB value of a pixel.
  @ingroup image_access
  @param   im Image.
  @param   i X pixel.
  @param   j Y pixel.
  @param[out]   r Red value.
  @param[out]   g Green value.
  @param[out]   b Blue value.
  @return  Whether coordinate is valid.
*/
int
vi_get_rgb(vimage *im, int i, int j,
           unsigned char *r, unsigned char *g, unsigned char *b)
{
    VI_CHECK(im);
    vi_need_rgb(im);

    if (!VALID(im, i, j))
        return 0;

    if (r != NULL)
        *r = RVAL(im, i, j);

    if (g != NULL)
        *g = GVAL(im, i, j);

    if (b != NULL)
        *b = BVAL(im, i, j);

    return 1;
}

/*!
  @brief   Get RGB image data.
  @ingroup image_access
  @param   im Image.
  @return  RGB data (don't deallocate).
  @note    You can use vi_rgb_index() to access pixel values.
*/
unsigned char *
vi_get_rgbdata(vimage *im)
{
    VI_CHECK(im);
    vi_need_rgb(im);
    return im->rgb;
}

/*!
  @brief   Get a row of RGB image data.
  @ingroup image_access
  @param   im Image.
  @param   row Row number.
  @return  RGB row (don't deallocate).
*/
unsigned char *
vi_get_rgbrow(vimage *im, int row)
{
    VI_CHECK(im);
    vi_need_rgb(im);
    return im->rgb + RV(im, 0, row);
}

/*!
  @brief   Return image width.
  @ingroup image_access
  @param   im Image.
  @return  Width.
*/
int
vi_get_width(vimage *im)
{
    VI_CHECK(im);
    return im->width;
}

/* Get a byte from a stream */
static unsigned char
vi_getbyte(FILE * fp)
{
    unsigned char byte;

    (void) fread(&byte, 1, 1, fp);
    return byte;
}

/* Get a long from a stream */
static int
vi_getlong(FILE * fp)
{
    unsigned char buf[4];

    (void) fread(buf, 4, 1, fp);
    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
}

/* Get a short from a stream */
static unsigned short
vi_getshort(FILE * fp)
{
    unsigned char buf[2];

    (void) fread(buf, 2, 1, fp);
    return (buf[0] << 8) + (buf[1] << 0);
}

/*!
  @brief   Return whether image has gray data.
  @ingroup image_access
  @param   im Image.
  @return  Yes or no.
*/
int
vi_has_gray(vimage *im)
{
    VI_CHECK(im);
    return im->gray != NULL;
}

/*!
  @brief   Return whether image has RGB data.
  @ingroup image_access
  @param   im Image.
  @return  Yes or no.
*/
int
vi_has_rgb(vimage *im)
{
    VI_CHECK(im);
    return im->rgb != NULL;
}


/*!
  @brief   Return whether image has RGBA data.
  @ingroup image_access
  @param   im Image.
  @return  Yes or no.
*/
int
vi_has_rgba(vimage *im)
{
    VI_CHECK(im);
    return im->rgb != NULL && im->gray != NULL;
}

/*!
  @brief   Require gray data in an image.
  @ingroup image_modify
  @param   im Image.

  Create gray data in the image if it doesn't already exist.
*/
void
vi_need_gray(vimage *im)
{
    VI_CHECK(im);

    if (im->gray == NULL) {
        int size = im->width * im->height;
        im->gray = V_ALLOC(unsigned char, size);
        memset(im->gray, 0, (size_t) size);
    }
}


/*!
  @brief   Require RGB data in an image.
  @ingroup image_modify
  @param   im Image.

  Create RGB data in the image if it doesn't already exist.
*/
void
vi_need_rgb(vimage *im)
{
    VI_CHECK(im);

    if (im->rgb == NULL) {
        int size = im->width * im->height * 3;
        im->rgb = V_ALLOC(unsigned char, size);
        memset(im->rgb, 0, (size_t) size);
    }
}

/* Print contents of an image */
void
vi_print(vimage *im, FILE *fp)
{
    VI_CHECK(im);

    v_print_start();
    v_push_indent();

    v_print_type(vimage_type, im, fp);

    v_indent(fp);
    fprintf(fp, "%d x %d ", im->width, im->height);

    if      (vi_has_rgba(im))
        fprintf(fp, "RGBA\n");
    else if (vi_has_rgb(im))
        fprintf(fp, "RGB\n");
    else if (vi_has_gray(im))
        fprintf(fp, "GRAY\n");
    else
        fprintf(fp, "empty");

    v_pop_indent();
    v_print_finish();
}

/* Puts a byte into a stream */
static int
vi_putbyte(FILE * fp, unsigned char val)
{
    return fwrite(&val, 1, 1, fp);
}

/* Puts a long into a stream */
static int
vi_putlong(FILE * fp, unsigned int val)
{
    unsigned char buf[4];

    buf[0] = (val >> 24);
    buf[1] = (val >> 16);
    buf[2] = (val >> 8);
    buf[3] = (val >> 0);

    return fwrite(buf, 4, 1, fp);
}

/* Puts a short into a stream */
static int
vi_putshort(FILE * fp, unsigned short val)
{
    unsigned char buf[2];

    buf[0] = (val >> 8);
    buf[1] = (val >> 0);

    return fwrite(buf, 2, 1, fp);
}

/* Read image from a stream */
vimage *
vi_read(FILE *fp)
{
    int flag, width, height, compress;
    unsigned char *data = NULL;
    unsigned int size, csize;
    vimage *im;

    /* Read dimensions */
    if (!v_read_long(&width, fp))
        return NULL;

    if (!v_read_long(&height, fp))
        return NULL;

    /* Read compression flag */
    if (!v_read_long(&compress, fp))
        return NULL;

    im = vi_create(width, height);
    im->compress = compress;

    /* Read RGB data */
    if (!v_read_long(&flag, fp))
        goto fail;

    if (flag) {
        size = width * height * 3;

        if (compress) {
            if (!v_read_long(&csize, fp))
                goto fail;

            data = V_ALLOC(unsigned char, csize);
            if (!v_read_char_list(data, csize, fp))
                goto fail;

            if ((im->rgb = v_uncompress(data, csize, size)) == NULL)
                goto fail;

            V_DEALLOC(data);
        } else {
            vi_need_rgb(im);
            if (!v_read_char_list(im->rgb, size, fp))
                goto fail;
        }
    }

    /* Read gray data */
    if (!v_read_long(&flag, fp))
        return NULL;

    if (flag) {
        size = width * height;

        if (compress) {
            if (!v_read_long(&csize, fp))
                goto fail;

            data = V_ALLOC(unsigned char, csize);
            if (!v_read_char_list(data, csize, fp))
                goto fail;

            if ((im->gray = v_uncompress(data, csize, size)) == NULL)
                goto fail;

            V_DEALLOC(data);
        } else {
            vi_need_gray(im);
            if (!v_read_char_list(im->gray, width * height, fp))
                goto fail;
        }
    }

    return im;

  fail:
    vi_destroy(im);
    V_DEALLOC(data);

    return NULL;
}

/*!
  @brief   Read an image from an image file.
  @ingroup image_io
  @param   file File to read (or \c NULL for stdin).
  @param   format Format recognized by \c convert(1) (or \c NULL).
  @param   format Image format (or \c NULL).
  @return  Image.
  @retval  NULL if it failed.

  Read an image from a file in the given format.  If the format is \c NULL,
  it is deduced from the file suffix.

  @note Uses \c convert(1) from ImageMagick for external formats.
*/
vimage *
vi_read_file(char *file, char *format)
{
    char *path, buf[BUFSIZ], temp[80];
    vimage *im;
    FILE *fp;

    if        (FORMAT("ppm")) {
        IMPORT(im, file, vi_read_ppm, 1);
    } else if (FORMAT("pgm")) {
        IMPORT(im, file, vi_read_pgm, 1);
    } else if (FORMAT("sgi")) {
        IMPORT(im, file, vi_read_sgi, 1);
    } else {
        /* Convert to SGI format */
        if (file == NULL)
            file = "-";

        if ((path = v_tempfile("convert", temp)) == NULL) {
            vi_err("can't create temporary file");
            return NULL;
        }

        sprintf(buf, "%s '%s' sgi:%s %s", CONVERT, file, path, SILENT);

        if (system(buf) != 0) {
            vi_err("can't run converter");
            unlink(path);
            return NULL;
        }

        /* Read it */
        IMPORT(im, path, vi_read_sgi, 0);
        unlink(path);
    }

    return im;
}

/*!
  @brief   Read an image from a PGM stream.
  @ingroup image_io
  @param   fp Stream to read.
  @return  Image.
  @retval  NULL if it failed.
*/
vimage *
vi_read_pgm(FILE *fp)
{
    int width, height, maxval, ascii, i, j;
    char magic[3];
    vimage *im;

    /* Read magic string */
    if (fscanf(fp, "%2s", magic) < 1)
        return NULL;

    if      (V_STREQ(magic, "P2"))
        ascii = 1;
    else if (V_STREQ(magic, "P5"))
        ascii = 0;
    else
        return NULL;

    /* Read width and height */
    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &width) < 1)
        return NULL;

    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &height) < 1)
        return NULL;

    /* Read maximum gray value */
    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &maxval) < 1)
        return NULL;

    if (maxval != 255)
        return NULL;

    /* Read separator character */
    fgetc(fp);

    /* Create image */
    im = vi_create(width, height);
    vi_need_gray(im);

    /* Read image data */
    if (ascii) {
        for (j = im->height - 1; j >= 0; j--)
            for (i = 0; i < width; i++)
                if (fscanf(fp, " %d", im->gray + GV(im, i, j)) < 1)
                    return im;
    } else {
        for (i = im->height - 1; i >= 0; i--)
            if (fread(im->gray + GV(im, 0, i), 1,
                      (size_t) width, fp) != (size_t) width)
                return im;
    }

    return im;
}

/*!
  @brief   Read an image from a PPM stream.
  @ingroup image_io
  @param   fp Stream to read.
  @return  Image.
  @retval  NULL if it failed.
*/
vimage *
vi_read_ppm(FILE *fp)
{
    int width, height, maxval, ascii, i, j, k, size;
    char magic[3];
    vimage *im;

    /* Read magic string */
    if (fscanf(fp, "%2s", magic) < 1)
        return NULL;

    if      (V_STREQ(magic, "P3"))
        ascii = 1;
    else if (V_STREQ(magic, "P6"))
        ascii = 0;
    else
        return NULL;

    /* Read width and height */
    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &width) < 1)
        return NULL;

    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &height) < 1)
        return NULL;

    /* Read maximum gray value */
    vi_skip_comments(fp);
    if (fscanf(fp, " %d", &maxval) < 1)
        return NULL;

    if (maxval != 255)
        return NULL;

    /* Read separator character */
    fgetc(fp);

    /* Create image */
    im = vi_create(width, height);
    vi_need_rgb(im);

    /* Read image data */
    if (ascii) {
        for (j = im->height - 1; j >= 0; j--)
            for (i = 0; i < width; i++)
                for (k = 0; k < 3; k++)
                    if (fscanf(fp, " %d", im->rgb + RV(im, i, j) + k) < 1)
                        return im;
    } else {
        size = width * 3;
        for (i = im->height - 1; i >= 0; i--)
            if (fread(im->rgb + RV(im, 0, i), 1,
                      (size_t) size, fp) != (size_t) size)
                return im;
    }

    return im;
}


/*!
  @brief   Read an image from a SGI stream.
  @ingroup image_io
  @param   fp Stream to read.
  @return  Image.
  @retval  NULL if it failed.
*/
vimage *
vi_read_sgi(FILE *fp)
{
    unsigned int *starttab, *lengthtab, length, offset, pos;
    unsigned char *row, *rledata;
    int i, j, k, tablen, rowsize;
    sgiheader hdr;
    vimage *im;

    /* Read and check magic number */
    hdr.magic = vi_getshort(fp);
    if (hdr.magic != SGI_MAGIC) {
        vi_err("input is not an SGI image");
        return NULL;
    }

    /* Read header info */
    hdr.storage   = vi_getbyte(fp);
    hdr.bpp       = vi_getbyte(fp);
    hdr.dimension = vi_getshort(fp);
    hdr.width     = vi_getshort(fp);
    hdr.height    = vi_getshort(fp);
    hdr.channels  = vi_getshort(fp);
    hdr.minval    = vi_getlong(fp);
    hdr.maxval    = vi_getlong(fp);

    /* Read 4 ignored bytes */
    for (i = 0; i < 4; i++)
	vi_getbyte(fp);

    /* Read image name */
    for (i = 0; i < 80; i++)
        vi_getbyte(fp);

    /* Read colourmap ID */
    vi_getlong(fp);

    /* Read more dummy bytes */
    for (i = 0; i < 404; i++)
        vi_getbyte(fp);

    /* Read offset tables if required */
    if (hdr.storage) {
        tablen = hdr.height * hdr.channels;

        starttab = V_ALLOCA(unsigned int, tablen);
        for (i = 0; i < tablen; i++)
            starttab[i] = vi_getlong(fp);

        lengthtab = V_ALLOCA(unsigned int, tablen);
        for (i = 0; i < tablen; i++)
            lengthtab[i] = vi_getlong(fp);
    }

    /* Create image */
    im = vi_create(hdr.width, hdr.height);

    if (hdr.channels > 1)
        vi_need_rgb(im);

    if (hdr.channels != 3)
        vi_need_gray(im);

    /* Read image data */
    rowsize = im->width;
    row = V_ALLOCA(unsigned char, rowsize);

    if (hdr.storage)
        rledata = V_ALLOCA(unsigned char, im->width);

    for (k = 0; k < hdr.channels; k++) {
        for (j = 0; j < im->height; j++) {
            /* Get next row of image data */
            if (hdr.storage) {
                pos = j + k * im->height;
                offset = starttab[pos];
                length = lengthtab[pos];

                if (fseek(fp, offset, SEEK_SET) < 0) {
                    vi_err("can't seek to image data");
                    vi_destroy(im);
                    return NULL;
                }

                if (fread(rledata, 1, length, fp) < length) {
                    vi_err("incomplete RLE data");
                    vi_destroy(im);
                    return NULL;
                }

                if (!vi_expandrow(row, rledata, rowsize)) {
                    vi_err("invalid RLE row: %d, %d", j, k);
                    vi_destroy(im);
                    return NULL;
                }
            } else {
                if (fread(row, 1, (size_t) im->width, fp) < (size_t) im->width) {
                    vi_err("incomplete image data");
                    vi_destroy(im);
                    return NULL;
                }
            }

            /* Add it to image */
            for (i = 0; i < im->width; i++) {
                if (hdr.channels == 3 || k < hdr.channels - 1)
                    VAL(im, i, j, k) = row[i];
                else
                    GRAY(im, i, j) = row[i];
            }
        }
    }

    return im;
}

/*!
  @brief   Set gray value of a pixel.
  @ingroup image_modify
  @param   im Image.
  @param   i X pixel.
  @param   j Y pixel.
  @param   val Value.
  @return  Whether coordinate is valid.
*/
int
vi_set_gray(vimage *im, int i, int j, unsigned char val)
{
    VI_CHECK(im);
    vi_need_gray(im);

    if (!VALID(im, i, j))
        return 0;

    GRAY(im, i, j) = val;
    return 1;
}

/*!
  @brief   Set gray data.
  @ingroup image_modify
  @param   im Image.
  @param   data Grayscale data.
*/
void
vi_set_graydata(vimage *im, unsigned char *data)
{
    VI_CHECK(im);
    vi_need_gray(im);

    memcpy(im->gray, data, (size_t) (im->width * im->height));
}

/*!
  @brief   Set RGB value of a pixel.
  @ingroup image_modify
  @param   im Image.
  @param   i X pixel.
  @param   j Y pixel.
  @param   r Red value.
  @param   g Green value.
  @param   b Blue value.
  @return  Whether coordinate is valid.
*/
int
vi_set_rgb(vimage *im, int i, int j,
           unsigned char r, unsigned char g, unsigned char b)
{
    VI_CHECK(im);
    vi_need_rgb(im);

    if (!VALID(im, i, j))
        return 0;

    SET_RGB(im, i, j, r, g, b);
    return 1;
}

/*!
  @brief   Set RGB data.
  @ingroup image_modify
  @param   im Image.
  @param   data RGB data.
*/
void
vi_set_rgbdata(vimage *im, unsigned char *data)
{
    VI_CHECK(im);
    vi_need_rgb(im);

    memcpy(im->rgb, data, (size_t) (im->width * im->height * 3));
}

/* Skip over hash comments in a stream */
static void 
vi_skip_comments(FILE *fp)
{
    int c;

    /* Skip whitespace */
    do {
        c = fgetc(fp);
    } while (isspace(c) && c != EOF);

    /* If comment line, read it */
    if (c == '#') {
        while ((c = fgetc(fp)) != '\n' && c != EOF)
            ;
    } else {
        ungetc(c, fp);
    }
}

/* Thaw an image from file */
vimage *
vi_thaw(FILE *fp)
{
    int i = 0, j = 0, width, height, count = 0, end = 0, channels;
    unsigned char values[4];
    vimage *im = NULL;

    v_thaw_start();

    if (!v_thaw_follow(fp, V_TOKEN_ID, "RGB, RGBA, GRAY or NONE"))
        return NULL;

    if        (V_STREQ(v_thaw_svalue, "RGB")) {
        channels = 3;
    } else if (V_STREQ(v_thaw_svalue, "RGBA")) {
        channels = 4;
    } else if (V_STREQ(v_thaw_svalue, "GRAY")) {
        channels = 1;
    } else if (V_STREQ(v_thaw_svalue, "NONE")) {
        channels = 0;
    } else {
        v_thaw_expected("RGB, RGBA, GRAY or NONE");
        goto fail;
    }

    if (!v_thaw_int(fp, &width))
        goto fail;

    if (!v_thaw_int(fp, &height))
        goto fail;

    im = vi_create(width, height);

    if (channels > 0) {
        if (!v_thaw_follow(fp, '{', "open-brace"))
            goto fail;

        if (channels > 1)
            vi_need_rgb(im);

        if (channels != 3)
            vi_need_gray(im);

        while (!end) {
            switch (v_thaw_token(fp)) {
            case V_TOKEN_INT:
                values[count] = (unsigned char) v_thaw_ivalue;
                if (++count < channels)
                    continue;

                if (VALID(im, i, j)) {
                    if (channels > 1)
                        SET_RGB(im, i, j,
                                values[0], values[1], values[2]);

                    if (channels != 3)
                        GRAY(im, i, j) = values[channels - 1];
                }

                count = 0;
                i++;
                break;
            case ',':
                i = count = 0;
                j++;
                break;
            case '}':
                end = 1;
                break;
            default:
                v_thaw_err("expected value, comma or close-brace");
                goto fail;
            }
        }
    }

    v_thaw_finish();
    return im;

  fail:
    if (im != NULL)
        vi_destroy(im);

    v_thaw_finish();
    return NULL;
}

/* Traverse an image */
int
vi_traverse(vimage *im, int (*func)(void *ptr))
{
    VI_CHECK(im);

    if ((*func)(im))
        return 1;

    v_push_traverse(im);
    v_pop_traverse();

    return 0;
}

/* Write image to a stream */
int
vi_write(vimage *im, FILE *fp)
{
    unsigned long size, csize;
    unsigned char *data;
    int flag;

    VI_CHECK(im);

    /* Dimensions */
    if (!v_write_long(im->width, fp))
        return 0;

    if (!v_write_long(im->height, fp))
        return 0;

    /* Compression flag */
    if (!v_write_long(im->compress, fp))
        return 0;

    /* RGB image */
    flag = (im->rgb != NULL);
    if (!v_write_long(flag, fp))
        return 0;

    if (flag) {
        size = im->width * im->height * 3;

        if (im->compress) {
            if ((data = v_compress(im->rgb, size, &csize)) == NULL)
                return 0;

            if (!v_write_long(csize, fp)) {
                V_DEALLOC(data);
                return 0;
            }
            
            if (!v_write_char_list(data, csize, fp)) {
                V_DEALLOC(data);
                return 0;
            }

            V_DEALLOC(data);
        } else {
            if (!v_write_char_list(im->rgb, size, fp))
                return 0;
        }
    }

    /* Gray image */
    flag = (im->gray != NULL);
    if (!v_write_long(flag, fp))
        return 0;

    if (flag) {
        size = im->width * im->height;

        if (im->compress) {
            if ((data = v_compress(im->gray, size, &csize)) == NULL)
                return 0;

            if (!v_write_long(csize, fp)) {
                V_DEALLOC(data);
                return 0;
            }
            
            if (!v_write_char_list(data, csize, fp)) {
                V_DEALLOC(data);
                return 0;
            }

            V_DEALLOC(data);
        } else {
            if (!v_write_char_list(im->gray, size, fp))
                return 0;
        }
    }

    return 1;
}

/*!
  @brief   Write an image to an image file.
  @ingroup image_io
  @param   im Image.
  @param   file File to write (or \c NULL for stdout).
  @param   format Format recognized by \c convert(1) (or \c NULL).
  @return  Whether successful.

  If \c format is \c NULL, the format is deduced from the file suffix (if
  any).

  @note Uses \c convert(1) from ImageMagick for external formats.
*/
int
vi_write_file(vimage *im, char *file, char *format)
{
    char buf[BUFSIZ];
    int ok = 0;
    FILE *fp;

    VI_CHECK(im);

    if (im->rgb == NULL && im->gray == NULL) {
        vi_err("image has no RGB or gray data");
        return 0;
    }

    if        (FORMAT("ppm")) {
        if (file == NULL) {
            ok = vi_write_ppm(im, stdout);
        } else if ((fp = fopen(file, "wb")) != NULL) {
            ok = vi_write_ppm(im, fp);
            fclose(fp);
        } else {
            vi_err("can't write to '%s': %s", file, strerror(errno));
        }
    } else if (FORMAT("pgm")) {
        if (file == NULL) {
            ok = vi_write_pgm(im, stdout);
        } else if ((fp = fopen(file, "wb")) != NULL) {
            ok = vi_write_pgm(im, fp);
            fclose(fp);
        } else {
            vi_err("can't write to '%s': %s", file, strerror(errno));
        }
    } else if (FORMAT("sgi")) {
        if (file == NULL) {
            ok = vi_write_sgi(im, stdout);
        } else if ((fp = fopen(file, "wb")) != NULL) {
            ok = vi_write_sgi(im, fp);
            fclose(fp);
        } else {
            vi_err("can't write to '%s': %s", file, strerror(errno));
        }
    } else {
        if (file == NULL)
            file = "-";

        if (format != NULL)
            sprintf(buf, "%s sgi:- '%s:%s' %s", CONVERT, format, file, SILENT);
        else
            sprintf(buf, "%s sgi:- '%s' %s", CONVERT, file, SILENT);

        if ((fp = popen(buf, "w")) == NULL) {
            vi_err("can't convert image");
            return 0;
        }

        if ((ok = vi_write_sgi(im, fp)) == 0)
            vi_err("can't write image data");

        pclose(fp);
    }

    return ok;
}

/*!
  @brief   Write gray data to PGM.
  @ingroup image_io
  @param   im Image.
  @param   fp Stream to write.
  @return  Whether successful.
*/
int
vi_write_pgm(vimage *im, FILE *fp)
{
    int i, size;

    VI_CHECK(im);
    vi_need_gray(im);

    fprintf(fp, "P5\n%d\n%d\n255\n", im->width, im->height);

    size = im->width;
    for (i = im->height - 1; i >= 0; i--)
        if (fwrite(im->gray + GV(im, 0, i), 1,
                   (size_t) size, fp) != (size_t) size)
            return 0;

    return 1;
}

/*!
  @brief   Write RGB data to PPM.
  @ingroup image_io
  @param   im Image.
  @param   fp Stream to write.
  @return  Whether successful.
*/
int
vi_write_ppm(vimage *im, FILE *fp)
{
    int i, size;

    VI_CHECK(im);
    vi_need_rgb(im);

    fprintf(fp, "P6\n%d\n%d\n255\n", im->width, im->height);

    size = im->width * 3;
    for (i = im->height - 1; i >= 0; i--)
        if (fwrite(im->rgb + RV(im, 0, i), 1,
                   (size_t) size, fp) != (size_t) size)
            return 0;

    return 1;
}

/*!
  @brief   Write image data to SGI stream.
  @ingroup image_io
  @param   im Image.
  @param   fp Stream to write.
  @return  Whether successful.
*/
int
vi_write_sgi(vimage *im, FILE *fp)
{
    unsigned char *row;
    sgiheader hdr;
    int i, j, k;

    /* Build SGI header */
    hdr.magic = SGI_MAGIC;
    hdr.storage = 0;
    hdr.bpp = 1;
    hdr.dimension = 3;

    hdr.width = im->width;
    hdr.height = im->height;

    if      (im->rgb != NULL && im->gray != NULL)
        hdr.channels = 4;
    else if (im->rgb != NULL)
        hdr.channels = 3;
    else if (im->gray != NULL)
        hdr.channels = 1;
    else
        return 0;

    hdr.minval = 0;
    hdr.maxval = 255;

    /* Write it */
    vi_putshort(fp, hdr.magic);
    vi_putbyte(fp,  hdr.storage);
    vi_putbyte(fp,  hdr.bpp);
    vi_putshort(fp, hdr.dimension);
    vi_putshort(fp, hdr.width);
    vi_putshort(fp, hdr.height);
    vi_putshort(fp, hdr.channels);
    vi_putlong(fp,  hdr.minval);
    vi_putlong(fp,  hdr.maxval);

    /* Write dummy bytes */
    for (i = 0; i < 4; i++)
	vi_putbyte(fp, 0);

    for (i = 0; i < 80; i++)
        vi_putbyte(fp, 0);

    vi_putlong(fp, 0);

    for (i = 0; i < 404; i++)
        vi_putbyte(fp, 0);

    /* Write image data */
    row = V_ALLOCA(unsigned char, im->width);

    for (k = 0; k < hdr.channels; k++) {
        for (j = 0; j < im->height; j++) {
            /* Build data row */
            for (i = 0; i < im->width; i++) {
                if (hdr.channels == 3 || k < hdr.channels - 1)
                    row[i] = VAL(im, i, j, k);
                else
                    row[i] = GRAY(im, i, j);
            }

            /* Write it */
            if (fwrite(row, (size_t) im->width, 1, fp) < 1)
                return 0;
        }
    }

    return 1;
}
