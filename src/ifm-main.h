/* Main function header */

#ifndef IFM_MAIN_H
#define IFM_MAIN_H

/* Output types */
enum {
    OUT_TEXT, OUT_WARNING, OUT_ERROR, OUT_DEBUG
};

#define debug(fmt, ...)   do_output(OUT_DEBUG,   fmt, ##__VA_ARGS__)
#define err(fmt, ...)     do_output(OUT_ERROR,   fmt, ##__VA_ARGS__)
#define output(fmt, ...)  do_output(OUT_TEXT,    fmt, ##__VA_ARGS__)
#define warn(fmt, ...)    do_output(OUT_WARNING, fmt, ##__VA_ARGS__)

/* External vars */
extern int line_number;
extern char *ifm_format;
extern vlist *ifm_search;
extern vlist *ifm_styles;

/* Advertised functions */
extern int run_main(int argc, char *argv[]);
extern void run_command(char *command);
extern int parse_input(char *file, int libflag, int required);
extern void do_output(int type, char *fmt, ...);
extern void set_output(void (*func)(int type, char *msg));
extern void yyerror(char *msg);

#endif
