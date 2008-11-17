/* Freeze/thaw scanner header */

#ifndef VARS_FREEZE_LEX_H
#define VARS_FREEZE_LEX_H

extern int start_line;
extern int line_number;
extern int last_line_number;

extern char *thaw_text(void);
extern int thaw_token(FILE *fp);
extern void thaw_restart(void);

#endif
