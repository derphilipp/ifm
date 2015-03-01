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

#include "ifm-driver.h"
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

#define STRINGIFY_DEF(name) STRINGIFY(name)
#define STRINGIFY(name) #name

/* Whether any output is required */
#define OUTPUT (write_map || write_items || write_tasks)

char *ifm_format = NULL;        /* Output format name */
int line_number;                /* Current line number */

vlist *ifm_search = NULL;       /* Search path */
vlist *ifm_styles = NULL;       /* Global styles */

static char *progname;          /* Program name */
static char infile[BUFSIZ];     /* Input filename */

static int write_map = 0;       /* Whether to write map */
static int write_items = 0;     /* Whether to write item list */
static int write_tasks = 0;     /* Whether to write task list */

static int driver_idx = -1;     /* Output driver index */
static int errors = 0;          /* No. of errors */
static int nowarn = 0;          /* Whether to suppress warnings */
static int max_errors = 10;     /* Print this many errors before abort */

static vlist *sections = NULL;  /* List of map sections to output */

/* Output handlers */
static void (*output_func)(int type, char *msg);

/* Internal functions */
static void print_version(void);
static int select_format(char *str);
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
    { NULL,   NULL,                NULL }
};

/* Main routine */
int
main(int argc, char *argv[])
{
    return run_main(argc, argv);
}

/* Main function */
int
run_main(int argc, char *argv[])
{
    char *env, *file = NULL, *info = NULL, *spec, *format = NULL;
    vlist *args, *list, *include = NULL, *vars = NULL;
    int noinit = 0, version = 0, help = 0, debug = 0;
    vhash *opts;
    V_BUF_DECL;
    viter iter;

#ifdef BISON_DEBUG
    extern int yydebug;
#endif

#ifdef FLEX_DEBUG
    extern int yy_flex_debug;
    yy_flex_debug = 0;
#endif

    /* Initialise */
    v_debug_env();
    progname = argv[0];

    /* Define options */
    v_initopts();

    v_optgroup("Output options:");

    v_option('m', "map", V_OPT_OPTARG, "sections",
             "Select map output");

    v_option_flag('i', "items", &write_items,
                  "Select item output");

    v_option_flag('t', "tasks", &write_tasks,
                  "Select task output");

    v_option_string('f', "format", "name", &format,
                    "Select output format");

    v_option_string('o', "output", "file", &file,
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

    v_option_flag('w', "nowarn", &nowarn,
                  "Don't print warnings");

    v_option_int('e', "errors", "num", &max_errors,
                 "Max errors before giving up (default: %d)", max_errors);

    v_optgroup("Information options:");

    v_option_string('\0', "show", "type", &info,
                    "Show information");

    v_option_flag('v', "version", &version,
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

    if (vh_exists(opts, "map")) {
        write_map = 1;
        spec = vh_sgetref(opts, "map");
        if (strlen(spec) > 0 && (sections = vl_parse_list(spec)) == NULL)
            fatal("invalid map section spec: %s", spec);
    }

    if (format != NULL)
        driver_idx = select_format(format);

    if (ifm_styles != NULL) {
        v_iterate(ifm_styles, iter)
            ref_style(vl_iter_svalref(iter));
    }

    /* Set search path if required */
    if (ifm_search != NULL)
        vl_destroy(ifm_search);

    ifm_search = vl_create();
    vl_spush(ifm_search, STRINGIFY_DEF(IFMLIB));

    if ((env = getenv("IFMPATH")) != NULL) {
        list = vl_split(env, PATHSEP);
        while (vl_length(list) > 0)
            vl_sunshift(ifm_search, vl_spop(list));
    }

    if (include != NULL)
        while (vl_length(include) > 0)
            vl_sunshift(ifm_search, vl_spop(include));

    /* Set internal debugging options */
#ifdef FLEX_DEBUG
    if (debug & 1)
        yy_flex_debug = 1;
#endif

#ifdef BISON_DEBUG
    if (debug & 2)
        yydebug = 1;
#endif

    /* Initialise map stuff */
    init_map();

    /* Parse system init file */
    if (!parse_input(SYSINIT, 1, 1))
        return 1;

    /* Parse personal init file(s) if available */
    if (!noinit) {
        char *home = getenv("HOME");

        if (home == NULL) {
            warn("HOME not set; using current directory");
            home = ".";
        }

        V_BUF_SETF("%s/.ifmrc", home);
        if (!parse_input(V_BUF_VAL, 0, 0))
            return 1;

        V_BUF_SETF("%s/ifm.ini", home);
        if (!parse_input(V_BUF_VAL, 0, 0))
            return 1;
    }

    /* Parse input files (or stdin) */
    args = v_getargs(opts);
    if (vl_length(args) > 0) {
        v_iterate(args, iter) {
            file = vl_iter_svalref(iter);
            parse_input(file, 0, 1);
        }
    } else if (info == NULL && !parse_input(NULL, 0, 1)) {
        return 1;
    }

    /* Load style definitions */
    set_style_list(ifm_styles);
    load_styles();
    if (errors)
        return 1;

    /* Set any variables from command line */
    if (vars != NULL) {
        char *cp;
        v_iterate(vars, iter) {
            spec = vl_iter_svalref(iter);
            if ((cp = strchr(spec, '=')) != NULL) {
                *cp++ = '\0';
                var_set(NULL, spec, vs_screate(cp));
            }
        }
    }

    /* Set output format if not already specified */
    if (OUTPUT && driver_idx < 0)
        driver_idx = select_format(NULL);

    /* Open output file if required */
    if (vh_exists(opts, "output")) {
        file = vh_sgetref(opts, "output");
        if (freopen(file, "w", stdout) == NULL)
            fatal("can't open %s", file);
    }

    /* Resolve tags */
    resolve_tags();
    if (errors)
        return 1;

    /* Set up rooms */
    setup_rooms();

    /* Set up links */
    setup_links();
    if (errors)
        return 1;

    /* Set up room exits */
    setup_exits();

    /* Set up map sections */
    setup_sections();

    /* Connect rooms together */
    connect_rooms();
    if (errors)
        return 1;

    /* Set up tasks */
    setup_tasks();
    if (errors)
        return 1;

    /* Solve game if required */
    if (!OUTPUT || write_tasks) {
        check_cycles();
        if (!errors)
            solve_game();
        else
            return 1;
    }

    /* Do what's required */
    if (driver_idx >= 0)
        ifm_format = drivers[driver_idx].name;

    /* Just show info if required */
    if (info == NULL) {
        if (!OUTPUT && !TASK_VERBOSE)
            output("Syntax appears OK\n");

        if (OUTPUT)
            print_start(driver_idx);

        if (write_map)
            print_map(driver_idx, sections);

        if (write_items)
            print_items(driver_idx);

        if (write_tasks)
            print_tasks(driver_idx);

        if (OUTPUT)
            print_finish(driver_idx);
    } else {
        show_info(info);
    }

    /* Er... that's it */
    return 0;
}

/* Run a command and return its output */
void
run_command(char *command)
{
    vlist *parts;
    int argc, i;
    char **argv;

    /* Split command into bits */
    parts = vl_qsplit(command, NULL, "'");

    /* Build command args */
    argc = vl_length(parts) + 1;
    argv = V_ALLOCA(char *, argc + 2);
    argv[0] = "ifm";

    for (i = 1; i <= argc; i++)
        argv[i] = vl_sgetref(parts, i - 1);

    argv[argc + 1] = NULL;

    /* Run command */
    run_main(argc, argv);

    /* Clean up */
    vl_destroy(parts);
}

/* Parse input from a file */
int
parse_input(char *file, int libflag, int required)
{
    void yyrestart(FILE *input_file);
    int yyparse(void);

    static int parses = 0;
    extern FILE *yyin;
    char *path = file;

    line_number = 0;

    if (file == NULL || V_STREQ(file, "-")) {
        strcpy(infile, "<stdin>");
        path = NULL;
    } else {
        if (libflag)
            path = find_file(file);

        if (!required && (path == NULL || !v_exists(path)))
            return 1;

        if (path == NULL)
            fatal("can't locate file '%s'", file);
        else if (!v_exists(path))
            fatal("file '%s' not found", path);

        strcpy(infile, path);
    }

    line_number = 1;
    errors = 0;

    if (path == NULL)
        yyin = stdin;
    else if ((yyin = fopen(path, "r")) == NULL)
        fatal("can't read '%s'", path);

    if (parses++)
        yyrestart(yyin);

    yyparse();

    fclose(yyin);
    line_number = 0;
    strcpy(infile, "");

    return (errors == 0);
}

/* Select an output format */
static int
select_format(char *str)
{
    int i, match = 0, nmatch = 0, len = 0;

    if (str != NULL)
        len = strlen(str);

    for (i = 0; drivers[i].name != NULL; i++) {
        if (str == NULL) {
            if (write_map && drivers[i].mfunc != NULL)
                return i;

            if (write_items && drivers[i].ifunc != NULL)
                return i;

            if (write_tasks && drivers[i].tfunc != NULL)
                return i;
        } else {
            if (strcmp(drivers[i].name, str) == 0)
                return i;

            if (strncmp(drivers[i].name, str, len) == 0) {
                nmatch++;
                match = i;
            }
        }
    }

    if (str == NULL)
        fatal("internal: no output format found");
    else if (nmatch == 0)
        fatal("unknown output format: %s", str);
    else if (nmatch > 1)
        fatal("ambiguous output format: %s", str);

    return match;
}

/* Write output */
void
do_output(int type, char *fmt, ...)
{
    errfuncs *func = NULL;
    V_BUF_DECL;
    char *msg;

    V_BUF_INIT;

    switch (type) {
    case OUT_ERROR:
    case OUT_FATAL:
        errors++;
        /* fallthrough */

    case OUT_WARNING:
        if (strlen(infile) > 0) {
            V_BUF_ADD(infile);
            if (line_number > 0)
                V_BUF_ADDF(", line %d", line_number);
            V_BUF_ADD(": ");
        }

        break;
    }

    V_ALLOCA_FMT(msg, fmt);
    msg = V_BUF_ADD(msg);

    if (output_func != NULL) {
        output_func(type, msg);
        return;
    }

    switch (type) {

    case OUT_TEXT:
        printf("%s", msg);
        break;

    case OUT_WARNING:
        if (!nowarn) {
            if (driver_idx >= 0)
                func = drivers[driver_idx].efunc;

            if (func == NULL)
                fprintf(stderr, "%s\n", msg);
            else
                func->warning(infile, line_number, msg);
        }

        break;

    case OUT_ERROR:
        if (driver_idx >= 0)
            func = drivers[driver_idx].efunc;

        if (func == NULL) {
            fprintf(stderr, "%s\n", msg);
            if (max_errors > 0 && errors >= max_errors)
                fatal("too many errors.  Goodbye!");
        } else {
            func->error(infile, line_number, msg);
        }

        break;

    case OUT_FATAL:
        fprintf(stderr, "%s: fatal: %s\n", progname, msg);
        exit(1);
        break;

    case OUT_DEBUG:
        if (getenv("IFM_DEBUG")) {
            V_BUF_FMT(fmt, msg);
            fprintf(stderr, "IFM: %s\n", msg);
        }

        break;
    }
}

/* Set output handler */
void
set_output(void (*func)(int type, char *msg))
{
    output_func = func;
}

/* Print program version and exit */
static void
print_version(void)
{
    output("IFM version %s\n", VERSION);
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
    int i, match = 0, nmatch = 0, len = strlen(type);

    /* Find info type */
    for (i = 0; showopts[i].name != NULL; i++) {
        if (strncmp(showopts[i].name, type, len) == 0) {
            nmatch++;
            match = i;
        }
    }

    if (nmatch == 0)
        fatal("unknown info type: %s", type);
    else if (nmatch > 1)
        fatal("ambiguous info type: %s", type);

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
    for (i = 0; drivers[i].name != NULL; i++)
        output("    %-15s     %s\n", drivers[i].name, drivers[i].desc);

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
