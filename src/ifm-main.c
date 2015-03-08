/* Main functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <vars.h>

#include "ifm-format.h"
#include "ifm-main.h"
#include "ifm-map.h"
#include "ifm-path.h"
#include "ifm-task.h"
#include "ifm-util.h"
#include "ifm-vars.h"

#ifndef SYSINIT
#define SYSINIT "ifm-init.ifm"
#endif

#ifndef MINGW32
#define PATHSEP ":"
#else
#define PATHSEP ";"
#endif

#define CHECK_ERR(stmt) do {                                            \
    stmt;                                                               \
    if (errors)                                                         \
        return 1;                                                       \
} while (0)

#define STRINGIFY_DEF(name) STRINGIFY(name)
#define STRINGIFY(name) #name

/* Whether any output is required */
#define OUTPUT (write_map || write_items || write_tasks)

char *ifm_format = NULL;        /* Output format name */
int line_number = 0;            /* Current line number */

vlist *ifm_search = NULL;       /* Search path */
vlist *ifm_styles = NULL;       /* Global styles */

static char *progname;          /* Program name */
static char infile[BUFSIZ];     /* Input filename */

static int write_map = 0;       /* Whether to write map */
static int write_items = 0;     /* Whether to write item list */
static int write_tasks = 0;     /* Whether to write task list */

static int format_idx = -1;     /* Output format index */
static int errors = 0;          /* No. of errors */
static int verbose = 0;         /* Whether to be verbose */
static int nowarn = 0;          /* Whether to suppress warnings */

static vlist *sections = NULL;  /* List of map sections to output */

/* Output handler */
static void (*output_func)(int type, char *msg) = NULL;

/* Internal functions */
static void print_version(void);
static int select_format(char *name);
static void show_info(char *type);
static void show_maps(void);
static void show_path(void);
static void usage(void);

/* Info options */
static struct show_st {
    char *name, *desc;
    void (*func)(void);
} showopts[] = {
    { "maps", "Show map sections",      show_maps },
    { "vars", "Show defined variables", var_list  },
    { "path", "Show file search path",  show_path },
    { NULL,   NULL,                     NULL }
};

/* Main function */
int
main(int argc, char *argv[])
{
    char *env, *file, *outfile = NULL, *info = NULL, *spec, *format = NULL;
    vlist *args, *list, *include = NULL, *vars = NULL;
    int noinit = 0, version = 0, help = 0, debug = 0;
    vhash *opts;
    V_BUF_DECL;
    viter iter;

    /* Set up debugging options */
#ifdef BISON_DEBUG
    extern int yydebug;
#endif

#ifdef FLEX_DEBUG
    extern int yy_flex_debug;
    yy_flex_debug = 0;
#endif

#ifdef VARS_DEBUG
    v_debug_env();
#endif

    /* Set program name */
    progname = argv[0];

    /* Initialise */
    initialize();

    /* Define options */
    v_optgroup("Output options:");

    v_option('m', "map", V_OPT_OPTARG, "sections",
             "Select map output");

    v_option_flag('i', "items", &write_items,
                  "Select item output");

    v_option_flag('t', "tasks", &write_tasks,
                  "Select task output");

    v_option_string('f', "format", "name", &format,
                    "Select output format");

    v_option_string('o', "output", "file", &outfile,
                    "Write output to specified file");

    v_optgroup("Auxiliary options:");

    v_option_list('I', NULL, "dir", &include,
                  "Prepend directory to search path");

    v_option_list('S', "style", "name", &ifm_styles,
                  "Push a style onto the style list");

    v_option_list('s', "set", "var=val", &vars,
                  "Set a customization variable");

    v_option_flag('\0', "noinit", &noinit,
                  "Don't read personal init file");

    v_option_flag('v', "verbose", &verbose,
                  "Be verbose about things");

    v_option_flag('w', "nowarn", &nowarn,
                  "Don't print warnings");

    v_optgroup("Information options:");

    v_option_string('\0', "show", "type", &info,
                    "Show information");

    v_option_flag('\0', "version", &version,
                  "Print program version");

    v_option_flag('h', "help", &help,
                  "This help message");

    v_option_int('\0', "DEBUG", "flag", &debug, NULL);

    /* Parse command-line arguments */
    if ((opts = v_getopts(argc, argv)) == NULL)
        v_die("Type '%s -help' for help", progname);

    if (help) {
        usage();
        return 0;
    }

    if (version) {
        print_version();
        return 0;
    }

#ifdef FLEX_DEBUG
    if (debug & 1)
        yy_flex_debug = 1;
#endif

#ifdef BISON_DEBUG
    if (debug & 2)
        yydebug = 1;
#endif

    /* Get map sections to output */
    if (vh_exists(opts, "map")) {
        write_map = 1;
        spec = vh_sgetref(opts, "map");
        if (strlen(spec) > 0)
            CHECK_ERR(set_map_sections(spec));
    }

    /* Set output format */
    CHECK_ERR(set_output_format(format));

    /* Set search path */
    add_search_dir(STRINGIFY_DEF(IFMLIB), 0);

    if ((env = getenv("IFMPATH")) != NULL) {
        list = vl_split(env, PATHSEP);
        while (vl_length(list) > 0)
            add_search_dir(vl_spop(list), 1);
    }

    if (include != NULL)
        while (vl_length(include) > 0)
            add_search_dir(vl_spop(include), 1);

    /* Parse system init file */
    CHECK_ERR(read_input(SYSINIT, 1, 1));

    /* Parse personal init file(s) if available */
    if (!noinit) {
        char *home = getenv("HOME");

        if (home == NULL) {
            warn("HOME not set; using current directory");
            home = ".";
        }

        V_BUF_SETF("%s/.ifmrc", home);
        CHECK_ERR(read_input(V_BUF_VAL, 0, 0));

        V_BUF_SETF("%s/ifm.ini", home);
        CHECK_ERR(read_input(V_BUF_VAL, 0, 0));
    }

    /* Parse input files (or stdin) if required */
    args = v_getargs(opts);

    if (vl_length(args) > 0) {
        v_iterate(args, iter) {
            file = vl_iter_svalref(iter);
            read_input(file, 0, 1);
        }
    } else if (info == NULL) {
        read_input(NULL, 0, 1);
    }

    CHECK_ERR();

    /* Set any variables from command line */
    if (vars != NULL) {
        char *cp;
        v_iterate(vars, iter) {
            spec = vl_iter_svalref(iter);
            if ((cp = strchr(spec, '=')) != NULL) {
                *cp++ = '\0';
                set_variable(spec, cp);
            }
        }
    }

    /* Prepare output for writing */
    CHECK_ERR(prepare_output());

    /* Open output file if required */
    if (outfile != NULL && freopen(outfile, "w", stdout) == NULL) {
        err("can't open '%s'", outfile);
        return 1;
    }

    /* Write output */
    if (info != NULL)
        show_info(info);
    else
        write_output();

    /* Er... that's it */
    return 0;
}

/* Initialize everything */
void
initialize(void)
{
    extern void init_parser(void);

    init_map();
    init_tasks();
    init_vars();
    init_parser();

    strcpy(infile, "");
    line_number = 0;
    format_idx = -1;

    if (ifm_search != NULL)
        vl_destroy(ifm_search);

    ifm_search = vl_create();

    if (ifm_styles != NULL)
        vl_destroy(ifm_styles);

    ifm_styles = NULL;

    if (sections != NULL)
        vl_destroy(sections);

    sections = NULL;
}

/* Add to the search path */
void
add_search_dir(char *path, int prepend)
{
    if (prepend) {
        info("Prepending '%s' to search path", path);
        vl_sunshift(ifm_search, path);
    } else {
        info("Appending '%s' to search path", path);
        vl_spush(ifm_search, path);
    }
}

/* Add a style to the style list */
void
add_global_style(char *style)
{
    if (ifm_styles == NULL)
        ifm_styles = vl_create();

    vl_spush(ifm_styles, style);
}

/* Set the output map sections */
void
set_map_sections(char *spec)
{
    if (sections != NULL)
        vl_destroy(sections);

    if ((sections = vl_parse_list(spec)) == NULL)
        err("invalid map section spec: %s", spec);
}

/* Set a variable */
void
set_variable(char *name, char *value)
{
    info("Setting %s to '%s'", name, value);
    var_set(NULL, name, vs_screate(value));
}

/* Set what's required on output */
void
set_output_options(int map, int items, int tasks)
{
    write_map = map;
    write_items = items;
    write_tasks = tasks;
}

/* Set output format */
void
set_output_format(char *name)
{
    if (name != NULL)
        format_idx = select_format(name);
}

/* Set output handler */
void
set_output_handler(void (*func)(int type, char *msg))
{
    output_func = func;
}

/* Read input from a file */
int
read_input(char *file, int search, int required)
{
    void yyrestart(FILE *input_file);
    int yyparse(void);

    static int parses = 0;
    extern FILE *yyin;
    char *path = file;

    line_number = 0;
    strcpy(infile, "");

    if (file == NULL || V_STREQ(file, "-")) {
        strcpy(infile, "<stdin>");
        path = NULL;
    } else {
        if (search)
            path = find_file(file);

        if (!required && (path == NULL || !v_exists(path)))
            return 1;

        if (path == NULL)
            err("can't locate file '%s'", file);
        else if (!v_exists(path))
            err("file '%s' not found", path);
        else
            strcpy(infile, path);
    }

    if (errors)
        return 1;

    info("Reading '%s'", infile);

    line_number = 1;
    errors = 0;

    if (path == NULL)
        yyin = stdin;
    else if ((yyin = fopen(path, "r")) == NULL)
        err("can't read '%s'", path);

    if (errors == 0) {
        if (parses++)
            yyrestart(yyin);
        yyparse();
    }

    if (yyin != NULL)
        fclose(yyin);

    line_number = 0;
    strcpy(infile, "");

    return (errors == 0);
}

/* Prepare output for writing */
int
prepare_output(void)
{
    viter iter;

    /* Load style definitions */
    if (ifm_styles != NULL) {
        set_style_list(ifm_styles);
        v_iterate(ifm_styles, iter)
            ref_style(vl_iter_svalref(iter));
    }

    CHECK_ERR(load_styles());

    /* Resolve tags */
    CHECK_ERR(resolve_tags());

    /* Set up rooms */
    CHECK_ERR(setup_rooms());

    /* Set up links */
    CHECK_ERR(setup_links());

    /* Set up room exits */
    CHECK_ERR(setup_exits());

    /* Set up map sections */
    CHECK_ERR(setup_sections());

    /* Connect rooms together */
    CHECK_ERR(connect_rooms());

    /* Set up tasks */
    CHECK_ERR(setup_tasks());

    /* Solve game if required */
    if (!OUTPUT || write_tasks) {
        CHECK_ERR(check_cycles());
        CHECK_ERR(solve_game());
    }

    return 0;
}

/* Write required output */
void
write_output(void)
{

    if (OUTPUT && format_idx < 0)
        format_idx = select_format(NULL);

    if (format_idx >= 0)
        ifm_format = formats[format_idx].name;

    if (!OUTPUT && !TASK_VERBOSE)
        output("Syntax appears OK\n");

    if (OUTPUT)
        info("Using '%s' output format", ifm_format);

    if (OUTPUT)
        print_start(format_idx);

    if (write_map)
        print_map(format_idx, sections);

    if (write_items)
        print_items(format_idx);

    if (write_tasks)
        print_tasks(format_idx);

    if (OUTPUT)
        print_finish(format_idx);
}

/* Write a line of output */
void
do_output(int type, char *fmt, ...)
{
    errfuncs *func = NULL;
    V_BUF_DECL;
    char *msg;

    V_BUF_INIT;

    if (type == O_ERROR)
        errors++;

    if (type == O_ERROR || type == O_WARNING) {
        if (strlen(infile) > 0) {
            V_BUF_ADD(infile);
            if (line_number > 0)
                V_BUF_ADDF(", line %d", line_number);
            V_BUF_ADD(": ");
        }
    }

    V_ALLOCA_FMT(msg, fmt);
    msg = V_BUF_ADD(msg);

    if (output_func != NULL) {
        output_func(type, msg);
    } else {
        switch (type) {

        case O_TEXT:
            printf("%s", msg);
            break;

        case O_INFO:
            if (verbose)
                fprintf(stderr, "%s\n", msg);

            break;

        case O_WARNING:
            if (!nowarn) {
                if (format_idx >= 0)
                    func = formats[format_idx].efunc;

                if (func == NULL)
                    fprintf(stderr, "%s\n", msg);
                else
                    func->warning(infile, line_number, msg);
            }

            break;

        case O_ERROR:
            if (format_idx >= 0)
                func = formats[format_idx].efunc;

            if (func == NULL)
                fprintf(stderr, "%s\n", msg);
            else
                func->error(infile, line_number, msg);

            break;

        case O_DEBUG:
            if (getenv("IFM_DEBUG")) {
                V_BUF_FMT(fmt, msg);
                fprintf(stderr, "IFM: %s\n", msg);
            }

            break;
        }
    }
}

/* Return a version string */
char *
get_version(void)
{
#ifdef HGVERSION
    V_BUF_DECL;
    return V_BUF_SETF("%s [%s]", VERSION, HGVERSION);
#else
    return VERSION;
#endif
}

/* Return information about an output format */
int
get_format_info(int format, char **name, char **desc,
                int *maps, int *items, int *tasks)
{
    int i;

    for (i = 0; formats[i].name != NULL; i++) {
        if (i == format) {
            if (name != NULL)
                *name = formats[i].name;

            if (desc != NULL)
                *desc = formats[i].desc;

            if (maps != NULL)
                *maps = (formats[i].mfunc != NULL);

            if (items != NULL)
                *items = (formats[i].ifunc != NULL);

            if (tasks != NULL)
                *tasks = (formats[i].tfunc != NULL);

            return 1;
        }
    }

    return 0;
}

/* Select an output format */
static int
select_format(char *name)
{
    int i, match = -1, nmatch = 0;

    for (i = 0; formats[i].name != NULL; i++) {
        if (name == NULL) {
            if (write_map && formats[i].mfunc != NULL)
                return i;

            if (write_items && formats[i].ifunc != NULL)
                return i;

            if (write_tasks && formats[i].tfunc != NULL)
                return i;
        } else {
            if (strcmp(formats[i].name, name) == 0)
                return i;

            if (strncmp(formats[i].name, name, strlen(name)) == 0) {
                nmatch++;
                match = i;
            }
        }
    }

    if (name == NULL)
        err("internal: no output format found");
    else if (nmatch == 0)
        err("unknown output format: %s", name);
    else if (nmatch > 1)
        err("ambiguous output format: %s", name);

    return match;
}

/* Print program version and exit */
static void
print_version(void)
{
    output("IFM version %s\n", get_version());
    output("Copyright (C) Glenn Hutchings <%s>\n\n", PACKAGE_BUGREPORT);

    output("This program is free software; you can redistribute it and/or modify\n");
    output("it under the terms of the GNU General Public License as published by\n");
    output("the Free Software Foundation; either version 2, or (at your option)\n");
    output("any later version.\n\n");

    output("This program is distributed in the hope that it will be useful,\n");
    output("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    output("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    output("GNU General Public License for more details.\n\n");

    output("You should have received a copy of the GNU General Public License\n");
    output("along with this program; if not, write to the Free Software\n");
    output("Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n");
}

/* Show some information */
static void
show_info(char *type)
{
    int i, match, nmatch = 0, len = strlen(type);

    /* Find info type */
    for (i = 0; showopts[i].name != NULL; i++) {
        if (strncmp(showopts[i].name, type, len) == 0) {
            nmatch++;
            match = i;
        }
    }

    if (nmatch == 0)
        err("unknown info type: %s", type);
    else if (nmatch > 1)
        err("ambiguous info type: %s", type);
    else
        showopts[match].func();
}

/* Print map sections */
static void
show_maps(void)
{
    int num = 1, xlen, ylen;
    vlist *rooms;
    char *title;
    vhash *sect;
    viter iter;
    V_BUF_DECL;

    set_map_vars();

    output("%s\t%s\t%s\t%s\t%s\n",
           "No.", "Rooms", "Width", "Height", "Name");

    v_iterate(sects, iter) {
        sect = vl_iter_pval(iter);

        if (vh_exists(sect, "TITLE"))
            title = vh_sgetref(sect, "TITLE");
        else
            title = V_BUF_SETF("Map section %d", num);

        rooms = vh_pget(sect, "ROOMS");
        xlen = vh_iget(sect, "XLEN");
        ylen = vh_iget(sect, "YLEN");

        if (show_map_title && vh_exists(sect, "TITLE"))
            ylen++;

        output("%d\t%d\t%d\t%d\t%s\n",
               num++, vl_length(rooms), xlen, ylen, title);
    }
}

/* Print file search path */
static void
show_path(void)
{
    output("%s\n", vl_join(ifm_search, " "));
}

/* Print a usage message and exit */
static void
usage()
{
    int i;

    v_usage("Usage: %s [options] [file...]", progname);

    output("\nOutput formats (may be abbreviated):\n");
    for (i = 0; formats[i].name != NULL; i++)
        output("    %-15s     %s\n", formats[i].name, formats[i].desc);

    output("\nShow options (may be abbreviated):\n");
    for (i = 0; showopts[i].name != NULL; i++)
        output("    %-15s     %s\n", showopts[i].name, showopts[i].desc);
}

/* Parser-called parse error */
void
yyerror(char *msg)
{
    if (V_STREQ(msg, "parse error"))
        err("syntax error");
    else
        err(msg);
}
