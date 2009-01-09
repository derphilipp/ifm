/* Path function header */

#ifndef IFM_PATH_H
#define IFM_PATH_H

#define NOPATH -1

/* Advertised functions */
extern void connect_rooms(void);
extern int find_path(vhash *step, vhash *from, vhash *to);
extern vlist *get_path(vhash *step, vhash *room);
extern void init_path(vhash *room);
extern void modify_path(int print);
extern vlist *reachable_rooms(vhash *room);

#endif
