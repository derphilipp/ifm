/* Utility functions */

#include <stdio.h>
#include <stdarg.h>
#include "svg-attr.h"
#include "svg-object.h"
#include "svg-util.h"

/* Attribute key mappings */
static struct attrmap_st {
    char *key, *attr;
} attrmap[] = {
    { "PENCOLOUR", "stroke" },
    { "FILLCOLOUR", "fill" },
    { "LINEWIDTH", "stroke-width" },
    { "TEXTANCHOR", "text-anchor" },

    /* FINISH ME -- add more here */

    /* Terminator */
    { NULL, NULL }
};

/* Get SVG style of an object, or NULL */
char *
svg_get_style(vhash *object)
{
    static vlist *styles = NULL;
    char *val, *attr, *key;
    V_BUF_DECL;
    int i;

    vl_init(styles);

    for (i = 0; attrmap[i].key != NULL; i++) {
        key = attrmap[i].key;
        attr = attrmap[i].attr;
        val = vh_sget(object, key);
        if (strlen(val) > 0) {
            vl_spush(styles, V_BUF_SETF("%s: %s;", attr, val));
            svg_debug("%s = %s", attr, val);
        }
    }

    if (vl_length(styles) > 0)
        return vl_join(styles, " ");

    return NULL;
}

/* Get an attribute of an object */
vscalar *
svg_get_attr(vhash *object, char *attr)
{
    vhash *parent;

    while (!vh_exists(object, attr) &&
           (parent = vh_pget(object, "PARENT")) != NULL)
        object = parent;

#if 0
    if (!vh_exists(object, attr))
        svg_fatal("no such attribute: %s", attr);
#endif

    svg_debug("%s = %s", attr, vh_sget(object, attr));

    return vh_get(object, attr);
}

/* Get the root figure of an object */
vhash *
svg_get_figure(vhash *object)
{
    vhash *parent;

    while ((parent = vh_pget(object, "PARENT")) != NULL)
        object = parent;

    return object;
}

/* Return the height of a character in a given fontsize */
float
svg_get_height(vhash *figure, float fontsize)
{
    return fontsize * SVG_TEXT_HSCALE;
}

/* Return the width of a character in a given fontsize */
float
svg_get_width(vhash *figure, float fontsize)
{
    return fontsize * SVG_TEXT_WSCALE;
}

/* Print a debugging message */
void
svg_debug(char *fmt, ...)
{
    char *str;

    if (getenv("SVG_DEBUG") != NULL) {
        V_ALLOCA_FMT(str, fmt);
        fprintf(stderr, "SVG: %s\n", str);
    }
}

/* Print a fatal error and die */
void
svg_fatal(char *fmt, ...)
{
    char *str;

    V_ALLOCA_FMT(str, fmt);
    fprintf(stderr, "Svg fatal: %s\n", str);
    exit(2);
}
