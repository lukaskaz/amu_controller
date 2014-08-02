/*
    uC/OS III operation system for AMU 2 controller

 */


#ifndef MOD_AUXILIARY_H
#define MOD_AUXILIARY_H

#include <stdint.h>

#define MAX_VELOCITY_VALUE       100U

typedef struct {
    uint8_t lightLeftCurrState;
    uint8_t lightRightCurrState;
    uint8_t lightInnerCurrState;
    uint8_t lightOuterCurrState;
} lightCurrentState_t;


extern lightCurrentState_t lightCurrentState;

#endif
