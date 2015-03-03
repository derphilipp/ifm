/* Main function header */

#ifndef IFM_MAIN_H
#define IFM_MAIN_H

/* Output types */
enum {
    O_TEXT, O_INFO, O_WARNING, O_ERROR, O_DEBUG
};

#define output(fmt, ...)  do_output(O_TEXT,    fmt, ##__VA_ARGS__)
#define info(fmt, ...)    do_output(O_INFO,    fmt, ##__VA_ARGS__)
#define warn(fmt, ...)    do_output(O_WARNING, fmt, ##__VA_ARGS__)
#define err(fmt, ...)     do_output(O_ERROR,   fmt, ##__VA_ARGS__)
#define debug(fmt, ...)   do_output(O_DEBUG,   fmt, ##__VA_ARGS__)

/* External vars */
extern int line_number;
extern char *ifm_format;
extern vlist *ifm_search;
extern vlist *ifm_styles;

/* Advertised functions */
extern int run_main(int argc, char *argv[]);
extern void initialize(void);
extern void add_search_dir(char *path, int prepend);
extern int set_map_sections(char *spec);
extern void set_variable(char *driver, char *name, char *value);
extern void set_output_driver(char *name);
extern void set_output_handler(void (*func)(int type, char *msg));
extern void run_command(char *command);
extern int parse_input(char *file, int search, int required);
extern void do_output(int type, char *fmt, ...);
extern void yyerror(char *msg);

#endif
