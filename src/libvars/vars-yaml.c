/*!
  @defgroup yaml YAML functions
  @ingroup methods

  These functions read and write Vars objects in YAML format.

  @bug: memory access problems when reading YAML stream; strings in stream
  are checked via vl_check() etc, and cause memory overrun.
*/

/*!
  @defgroup yaml_high High-level functions
  @ingroup yaml

  These functions are for high-level use.
*/

/*!
  @defgroup yaml_low Low-level functions
  @ingroup yaml

  These functions are for use when defining new object types (see @ref
  extend).
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_LIBYAML
#include <yaml.h>
#endif

#include "vars-config.h"
#include "vars-buffer.h"
#include "vars-hash.h"
#include "vars-system.h"
#include "vars-type.h"
#include "vars-yaml.h"

#define TAG_PREFIX "vars/"

/* Output width */
#define WIDTH 65

#define GET_EVENT(parser, event)                            \
    do {                                                    \
        if (!yaml_parser_parse(parser, event)) {            \
            v_yaml_read_err(NULL, parser);                  \
            return NULL;                                    \
        }                                                   \
    } while (0)

#define NEXT_EVENT(parser, event)               \
    yaml_event_delete(event);                   \
    GET_EVENT(parser, event)

#define GET_TAG(item, buf)                                      \
    do {                                                        \
        if (item.tag != NULL)                                   \
            strcpy(buf, item.tag);                              \
        else                                                    \
            strcpy(buf, "");                                    \
    } while (0)

/* Current YAML filename */
static char *filename = NULL;

#ifdef HAVE_LIBYAML
/* Current YAML emitter */
static yaml_emitter_t emitter;
#endif

/* YAML writing depth */
static int depth = 0;

/* YAML error buffer */
V_NBUF_DECL(ebuf);

/* Internal functions */
#ifdef HAVE_LIBYAML
static void *v_yaml_read_object(yaml_parser_t *parser, yaml_event_t *event);
static void *v_yaml_import_object(void *ptr, char *tag);
static vscalar *v_yaml_scalar(void *ptr);
static void v_yaml_read_err(char *msg, yaml_parser_t *parser);
static int v_yaml_write_scalar(vscalar *s, FILE *fp);
static int v_yaml_write_list(vlist *l, char *tag, FILE *fp);
static int v_yaml_write_hash(vhash *h, char *tag, FILE *fp);
static int v_yaml_write_string(char *val, FILE *fp);
static int v_yaml_start(FILE *fp);
static int v_yaml_finish(FILE *fp);
static int v_yaml_start_list(FILE *fp, char *tag);
static int v_yaml_finish_list(FILE *fp);
static int v_yaml_start_hash(FILE *fp, char *tag);
static int v_yaml_finish_hash(FILE *fp);
static char *v_yaml_tag(char *tag);
#endif

/*!
  @brief   Read YAML objects from a file.
  @ingroup yaml_high
  @param   file File to read.
  @return  Pointer to Vars object.
  @retval  NULL if it failed.
*/
void *
v_yaml_read_file(char *file)
{
#ifdef HAVE_LIBYAML
    void *ptr = NULL;
    V_BUF_DECL;
    FILE *fp;

    if ((fp = v_open(file, "r")) != NULL) {
        filename = file;
        ptr = v_yaml_read(fp);
        filename = NULL;
        v_close(fp);
    } else {
        V_BUF_SETF("can't open %s: %s", file, strerror(errno));
        v_yaml_read_err(V_BUF_VAL, NULL);
    }

    return ptr;
#else
    v_unavailable("v_yaml_read_file()");    
#endif
}

/*!
  @brief   Read YAML objects from a stream.
  @ingroup yaml_high
  @param   fp Stream to read.
  @return  Pointer to Vars object.
  @retval  NULL if it failed.
*/
void *
v_yaml_read(FILE *fp)
{
#ifdef HAVE_LIBYAML
    vlist *objects = NULL;
    void *object = NULL;
    vscalar *value;

    yaml_parser_t parser;
    yaml_event_t event;

    /* Initalize */
    v_declare();

    if (yaml_parser_initialize(&parser)) {
        yaml_parser_set_input_file(&parser, fp);
    } else {
        v_yaml_read_err("can't initialize parser", NULL);
        return NULL;
    }

    /* Read initial YAML event */
    GET_EVENT(&parser, &event);

    /* Parse events and build objects */
    objects = vl_create();
    while ((object = v_yaml_read_object(&parser, &event)) != NULL) {
        if (vl_check(object) || vh_check(object)) {
            vl_ppush(objects, object);
        } else {
            value = v_yaml_scalar(object);
            vl_push(objects, value);
            NEXT_EVENT(&parser, &event);
        }
    }

    /* Clean up */
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);

    /* If more than one document, return list of them */
    if (vl_length(objects) > 1)
        return objects;

    /* Otherwise, extract the document and return it */
    value = vl_head(objects);
    if (vs_type(value) == V_TYPE_POINTER)
        object = vl_pshift(objects);
    else
        object = vl_shift(objects);

    vl_destroy(objects);
    return object;
#else
    v_unavailable("v_yaml_read()");
#endif
}

/*!
  @brief   Return last YAML parser error.
  @ingroup yaml_high
  @return  Error string.
  @retval  NULL if no errors found.
*/
char *
v_yaml_error(void)
{
    if (ebuf != NULL)
        return vb_get(ebuf);

    return NULL;
}

#ifdef HAVE_LIBYAML
/* Read a Vars object from a YAML parser */
static void *
v_yaml_read_object(yaml_parser_t *parser, yaml_event_t *event)
{
    void *object = NULL;
    char *key, *tag;
    vscalar *value;
    vlist *list;
    vhash *hash;

    while (1) {
        switch (event->type) {

        case YAML_MAPPING_START_EVENT:
            tag = V_STRDUP(event->data.mapping_start.tag);
            hash = vh_create();

            while (1) {
                NEXT_EVENT(parser, event);

                if (event->type == YAML_MAPPING_END_EVENT)
                    break;

                key = V_STRDUP(event->data.scalar.value);
                NEXT_EVENT(parser, event);
                if ((object = v_yaml_read_object(parser, event)) == NULL) {
                    v_yaml_read_err("incomplete hash", parser);
                    return NULL;
                }

                if (vl_check(object) || vh_check(object)) {
                    vh_pstore(hash, key, object);
                } else {
                    value = v_yaml_scalar(object);
                    vh_store(hash, key, value);
                }

                V_DEALLOC(key);
            }

            if (tag != NULL)
                object = v_yaml_import_object(hash, tag);
            else
                object = hash;

            V_DEALLOC(tag);
            return object;

        case YAML_SEQUENCE_START_EVENT:
            tag = V_STRDUP(event->data.sequence_start.tag);
            list = vl_create();

            while (1) {
                NEXT_EVENT(parser, event);
                if (event->type == YAML_SEQUENCE_END_EVENT)
                    break;

                if ((object = v_yaml_read_object(parser, event)) == NULL) {
                    v_yaml_read_err("incomplete list", parser);
                    return NULL;
                }

                if (vl_check(object) || vh_check(object)) {
                    vl_ppush(list, object);
                } else {
                    value = v_yaml_scalar(object);
                    vl_push(list, value);
                }
            }

            if (tag != NULL)
                object = v_yaml_import_object(list, tag);
            else
                object = list;

            V_DEALLOC(tag);
            return object;

        case YAML_SCALAR_EVENT:
            return vs_screate(event->data.scalar.value);

        case YAML_STREAM_END_EVENT:
            return NULL;

        default:
            NEXT_EVENT(parser, event);
            break;
        }
    }
}

/* Import an object based on its tag */
static void *
v_yaml_import_object(void *ptr, char *tag)
{
    static char buf[30];
    void *newptr;
    vtype *t;
    char *cp;

    strcpy(buf, tag);
    if ((tag = strstr(buf, TAG_PREFIX)) != NULL) {
        tag += strlen(TAG_PREFIX);
        for (cp = tag; *cp != '\0'; cp++)
            *cp = toupper(*cp);

        if      ((t = v_find_name(tag)) == NULL)
            v_fatal("unrecognized Vars type: %s", tag);
        else if (t->yamlimport == NULL)
            v_fatal("type %s has no YAML import function", tag);

        if ((newptr = t->yamlimport(ptr)) == NULL)
            v_fatal("invalid %s import function", tag);

        if      (vl_check(ptr))
            vl_destroy(ptr);
        else if (vh_check(ptr))
            vh_destroy(ptr);

        ptr = newptr;
    }

    return ptr;
}

/* Convert pointer to a scalar */
static vscalar *
v_yaml_scalar(void *ptr)
{
    vscalar *s = ptr;
    char *sval, *end;
    double dval;
    int ival;

    /* If scalar, try converting to number */
    if (vs_check(ptr)) {
        sval = vs_sgetref(s);

        /* Try double value */
        dval = strtod(sval, &end);
        if (*end == '\0') {
            vs_dstore(s, dval);
            return s;
        }

        /* Try int value */
        ival = strtol(sval, &end, 0);
        if (*end == '\0') {
            vs_istore(s, ival);
            return s;
        }

        /* Nope, it's a string */
        return s;
    }

    /* Otherwise, it's a Vars pointer */
    return vs_pcreate(ptr);
}

/* Give a YAML read error */
static void
v_yaml_read_err(char *msg, yaml_parser_t *parser)
{
    V_NBUF_SET(ebuf, filename ? filename : "<stdin>");

    if (parser != NULL)
        V_NBUF_ADDF(ebuf, ", line %d, column %d",
                    parser->problem_mark.line,
                    parser->problem_mark.column);

    if (msg != NULL)
        V_NBUF_ADDF(ebuf, ": %s", msg);

    if (parser != NULL)
        V_NBUF_ADDF(ebuf, ": %s", parser->problem);

    v_exception("%s", V_NBUF_VAL(ebuf));
}
#endif

/*!
  @brief   Write contents of a pointer in YAML format.
  @ingroup yaml_high
  @param   ptr Pointer to Vars object.
  @param   fp Stream to write.
  @return  Whether successful.
  @todo    Deal with multiply-referenced pointers, using anchors.
*/
int
v_yaml_write(void *ptr, FILE *fp)
{
#ifdef HAVE_LIBYAML
    void *object = NULL;
    char *tag = NULL;
    vtype *t;

    if (!v_yaml_start(fp))
        return 0;

    if (ptr != NULL) {
        /* If not list or hash, it must be exported */
        if (!vl_check(ptr) && !vh_check(ptr)) {
            /* Get pointer type */
            if ((t = v_type(ptr)) == NULL)
                v_fatal("v_yaml_write(): unknown pointer type");

            /* Get export function */
            if (t->yamlexport == NULL)
                v_fatal("v_yaml_write(): type %s has no YAML export function",
                        t->name);

            /* Export object to list or hash */
            object = t->yamlexport(ptr);

            /* See what we got */
            if (vl_check(object) || vh_check(object))
                ptr = object;
            else
                v_fatal("v_yaml_write(): invalid %s export function", t->name);

            /* Set the tag */
            tag = v_name(t);
        }

        /* Write object */
        if      (vl_check(ptr))
            v_yaml_write_list(ptr, tag, fp);
        else if (vh_check(ptr))
            v_yaml_write_hash(ptr, tag, fp);

        /* Clean up if required */
        if (object != NULL) {
            if (t->yamlcleanup != NULL)
                t->yamlcleanup(object);
            else
                v_destroy(object);
        }
    }

    if (!v_yaml_finish(fp))
        return 0;

    return 1;
#else
    v_unavailable("v_yaml_write()");
#endif
}

/*!
  @brief   Write contents of a pointer to a file in YAML format.
  @ingroup yaml_high
  @param   ptr Pointer to Vars object.
  @param   file File to write.
  @retval  Whether successful.
  @todo    Set error buffer on failure.
*/
int
v_yaml_write_file(void *ptr, char *file)
{
#ifdef HAVE_LIBYAML
    int ok = 0;
    V_BUF_DECL;
    FILE *fp;

    if ((fp = v_open(file, "w")) != NULL) {
        filename = file;
        ok = v_yaml_write(ptr, fp);
        filename = NULL;
        v_close(fp);
    } else {
        V_BUF_SETF("can't open %s: %s", file, strerror(errno));
    }

    return ok;
#else
    v_unavailable("v_yaml_write_file()");
#endif
}

#ifdef HAVE_LIBYAML
/* Write a scalar to YAML stream */
static int
v_yaml_write_scalar(vscalar *s, FILE *fp)
{
    int ok;

    switch (vs_type(s)) {

    case V_TYPE_POINTER:
        ok = v_yaml_write(vs_pget(s), fp);
        break;

    default:
        ok = v_yaml_write_string(vs_sget(s), fp);
        break;
    }

    return ok;
}

/* Write a list to YAML stream */
static int
v_yaml_write_list(vlist *l, char *tag, FILE *fp)
{
    vscalar *s;
    viter i;

    if (!v_yaml_start_list(fp, tag))
        return 0;

    v_iterate(l, i) {
        s = vl_iter_val(i);
        if (!v_yaml_write_scalar(s, fp))
            return 0;
    }

    if (!v_yaml_finish_list(fp))
        return 0;

    return 1;
}

/* Write a hash to YAML stream */
static int
v_yaml_write_hash(vhash *h, char *tag, FILE *fp)
{
    vscalar *s;
    char *key;
    viter i;

    if (!v_yaml_start_hash(fp, tag))
        return 0;

    v_iterate(h, i) {
        key = vh_iter_key(i);
        if (!v_yaml_write_string(key, fp))
            return 0;

        s = vh_iter_val(i);
        if (!v_yaml_write_scalar(s, fp))
            return 0;
    }

    if (!v_yaml_finish_hash(fp))
        return 0;

    return 1;
}

/* Write a string to YAML stream */
static int
v_yaml_write_string(char *val, FILE *fp)
{
    yaml_event_t event;

    if (!yaml_scalar_event_initialize(&event, NULL, NULL, val, strlen(val), 
                                      1, 1, YAML_ANY_SCALAR_STYLE))
        return 0;

    if (!yaml_emitter_emit(&emitter, &event))
        return 0;

    return 1;
}

/* Start writing YAML */
static int
v_yaml_start(FILE *fp)
{
    yaml_event_t event;

    if (depth++ == 0) {
        /* Initialize emitter */
        if (yaml_emitter_initialize(&emitter)) {
            yaml_emitter_set_output_file(&emitter, fp);
            yaml_emitter_set_width(&emitter, WIDTH);
        } else {
            depth = 0;
            return 0;
        }

        /* Write header */
        fprintf(fp, "%%YAML 1.1\n");
        fprintf(fp, "---\n");

        /* Send initial events */
        if (!yaml_stream_start_event_initialize(&event, YAML_ANY_ENCODING))
            goto fail;

        if (!yaml_emitter_emit(&emitter, &event))
            goto fail;

        if (!yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1))
            goto fail;

        if (!yaml_emitter_emit(&emitter, &event))
            goto fail;
    }

    return 1;

  fail:
    yaml_emitter_delete(&emitter);
    return 0;
}

/* Finish writing YAML */
static int
v_yaml_finish(FILE *fp)
{
    yaml_event_t event;

    if (--depth == 0) {
        /* Send final events */
        if (!yaml_document_end_event_initialize(&event, 1))
            goto fail;

        if (!yaml_emitter_emit(&emitter, &event))
            goto fail;

        if (!yaml_stream_end_event_initialize(&event))
            goto fail;

        if (!yaml_emitter_emit(&emitter, &event))
            goto fail;

        /* Clean up */
        yaml_emitter_delete(&emitter);
    }

    return 1;

  fail:
    yaml_emitter_delete(&emitter);
    return 0;
}

/* Write a list start marker to YAML stream */
static int
v_yaml_start_list(FILE *fp, char *tag)
{
    yaml_event_t event;

    tag = v_yaml_tag(tag);
    if (!yaml_sequence_start_event_initialize(&event, NULL, tag, tag == NULL,
                                              YAML_ANY_SEQUENCE_STYLE))
        return 0;

    if (!yaml_emitter_emit(&emitter, &event))
        return 0;

    return 1;
}

/* Write a list end marker to YAML stream */
static int
v_yaml_finish_list(FILE *fp)
{
    yaml_event_t event;

    if (!yaml_sequence_end_event_initialize(&event))
        return 0;

    if (!yaml_emitter_emit(&emitter, &event))
        return 0;

    return 1;
}

/* Write a hash start marker to YAML stream */
static int
v_yaml_start_hash(FILE *fp, char *tag)
{
    yaml_event_t event;

    tag = v_yaml_tag(tag);
    if (!yaml_mapping_start_event_initialize(&event, NULL, tag, tag == NULL,
                                             YAML_ANY_MAPPING_STYLE))
        return 0;

    if (!yaml_emitter_emit(&emitter, &event))
        return 0;

    return 1;
}

/* Write a hash end marker to YAML stream */
static int
v_yaml_finish_hash(FILE *fp)
{
    yaml_event_t event;

    if (!yaml_mapping_end_event_initialize(&event))
        return 0;

    if (!yaml_emitter_emit(&emitter, &event))
        return 0;

    return 1;
}

/* Return a Vars yaml tag */
static char *
v_yaml_tag(char *tag)
{
    static char buf[30];
    char *cp;

    if (tag != NULL) {
        sprintf(buf, "%s%s", TAG_PREFIX, tag);
        for (cp = buf; *cp != '\0'; cp++)
            *cp = tolower(*cp);
        tag = buf;
    }

    return tag;
}
#endif
