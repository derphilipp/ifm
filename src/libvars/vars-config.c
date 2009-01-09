/* Configuration functions */

#include <math.h>
#include "vars-config.h"

/* Replacement for missing drem() */
double
v_drem(double x, double y)
{
    return x - y * round(x / y);
}
