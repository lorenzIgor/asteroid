#ifndef _SHIP_H_
#define _SHIP_H_

#include <cglm/vec2.h>

enum SHIPSTATE
{
  HALTED,
  UTHRUST,
  DTHRUST,
  LTHRUST,
  RTHRUST,
  DAMAGED
};

#endif