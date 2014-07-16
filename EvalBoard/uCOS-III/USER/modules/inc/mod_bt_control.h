/*
    uC/OS III operation system for AMU 2 controller

 */


#ifndef MOD_BT_CONTROL_H
#define MOD_BT_CONTROL_H

#include <includes.h>

#define BT_BUFFER_SIZE_MAX      100U

typedef struct {
    uint8_t BTBuffer[BT_BUFFER_SIZE_MAX];
    uint8_t BTBufferElemNb;
} BTMessageStruct_t;

extern BTMessageStruct_t BTMessageStruct;

extern void AppTaskBTControl(void *p_arg);
extern void bt_message_receive_handler(void);

#endif
