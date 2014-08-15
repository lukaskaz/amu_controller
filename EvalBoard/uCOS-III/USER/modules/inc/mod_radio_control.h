/*
    uC/OS III operation system for AMU 2 controller

 */


#ifndef MOD_RADIO_CONTROL_H
#define MOD_RADIO_CONTROL_H

#include <includes.h>

#define OP_QUEUE_SIZE       10U

typedef enum {
    RADIO_OP_NONE = 0,
    RADIO_OP_DRIVE,
    RADIO_OP_LIGHTING,
    RADIO_OP_SOUND_SIG,
    RADIO_OP_HEARTBEAT,
} RadioOperations_t;

typedef enum {
    RADIO_DRVOP_STOPPED = 0,
    RADIO_DRVOP_FORWARD,
    RADIO_DRVOP_BACKWARD,
    RADIO_DRVOP_LEFT,
    RADIO_DRVOP_RIGHT,
    RADIO_DRVOP_JOY_STOPPED,
    RADIO_DRVOP_JOY_FORWARD,
    RADIO_DRVOP_JOY_BACKWARD,
    RADIO_DRVOP_JOY_LEFT,
    RADIO_DRVOP_JOY_RIGHT,
} radioDriveOperations_t;

typedef enum {
    RADIO_LIGHTST_DISABLE = 0,
    RADIO_LIGHTST_ENABLE,
} radioLightState_t;

typedef enum {
    RADIO_LIGHTOP_UNDEF = 0,
    RADIO_LIGHTOP_LEFT,
    RADIO_LIGHTOP_RIGHT,
    RADIO_LIGHTOP_INNER,
    RADIO_LIGHTOP_OUTER,
} LightOperation_t;

typedef enum {
    RADIO_SIGOP_UNDEF = 0,
    RADIO_SIGOP_ON,
} SignalOperation_t;

typedef enum {
    RADIO_CTRL_NONE = 0,
    RADIO_CTRL_CONSOLE,
    RADIO_CTRL_JOYSTICK,
    RADIO_CTRL_BLUETOOTH,
} RadioController_t;

typedef struct {
    uint8_t operation;
    uint8_t ctrl;
    uint8_t opAction;
    uint8_t val_0;
    uint8_t val_1;
} opQueueElem_t;

typedef struct {
    opQueueElem_t opQueueElem[OP_QUEUE_SIZE];
    uint8_t queueElemNb;
} opQueueStruct_t;

extern OS_Q opQueue;
extern opQueueStruct_t opQueueStruct;

extern void AppTaskRadioControl(void *p_arg);
extern void radio_frame_receive_handler(void);

extern void send_to_radio_queue(const opQueueElem_t *opQueueElemPtr);
extern opQueueElem_t *receive_from_radio_queue(void);
extern void flush_radio_queue(void);

#endif
