/* Main function header */

#ifndef IFM_MAIN_H
#define IFM_MAIN_H

/* External vars */
extern int line_number;
extern char *ifm_format;
extern vlist *ifm_search;
extern vlist *ifm_styles;

/* Advertised functions */
extern int parse_input(char *file, int libflag, int required);
extern void switch_file(char *file, int line, int flag);
extern void yyerror(char *msg);
extern void err(char *fmt, ...);
extern void warn(char *fmt, ...);
extern void debug(char *fmt, ...);
extern void fatal(char *fmt, ...);

#endif
