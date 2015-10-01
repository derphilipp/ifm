/* SVG output format header */

#ifndef IFM_SVG_H
#define IFM_SVG_H

#include "ifm-format.h"

extern mapfuncs svg_mapfuncs;

extern void svg_map_start(void);
extern void svg_map_section(vhash *sect);
extern void svg_map_room(vhash *room);
extern void svg_map_link(vhash *link);
extern void svg_map_endsection(void);
extern void svg_map_finish(void);

#endif
