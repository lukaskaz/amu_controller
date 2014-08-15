/*
    uC/OS III operation system for AMU 2 controller

 */


#ifndef MOD_LCD_H
#define MOD_LCD_H

#include <stdbool.h>
#include <includes.h>

#define LCD_QUEUE_SIZE       10U

typedef enum {
    LCD_CTRL_NONE = 0,
    LCD_CTRL_CONSOLE,
    LCD_CTRL_JOYSTICK,
    LCD_CTRL_BLUETOOTH,
} LcdControllers_t;

typedef enum {
    LCD_MV_STOPPED = 0,
    LCD_MV_FORWARD,
    LCD_MV_BACKWARD,
    LCD_MV_LEFT,
    LCD_MV_RIGHT,
} LcdMoveDirection_t;

typedef enum {
    LCD_LIG_UNDEF = 0,
    LCD_LIG_LEFT,
    LCD_LIG_RIGHT,
    LCD_LIG_INNER,
    LCD_LIG_OUTER,
} LcdLightType_t;

typedef enum {
    LCD_LIG_DISABLE = 0,
    LCD_LIG_ENABLE,
} LcdLightState_t;

typedef enum {
    LCD_HRN_OFF = 0,
    LCD_HRN_ON,
} LcdHornState_t;

typedef enum {
    LCD_CONN_OFFLINE = 0,
    LCD_CONN_ONLINE,
    LCD_CONN_FREE,
} LcdConnectionState_t;

typedef enum {
    LCD_OP_NONE,
    LCD_OP_MOVE,
    LCD_OP_LIGHT,
    LCD_OP_HORN,
    LCD_OP_CONN,
} LcdOperation_t;

typedef union {
    LcdOperation_t operation;

    struct {
        LcdOperation_t operation;
        LcdControllers_t ctrl;
        LcdMoveDirection_t dir;
        uint8_t speed;
    } move;

    struct {
        LcdOperation_t operation;
        LcdControllers_t ctrl;
        LcdLightType_t type;
        LcdLightState_t state;
    } light;

    struct {
        LcdOperation_t operation;
        LcdControllers_t ctrl;
        LcdHornState_t state;
    } horn;

    struct {
        LcdOperation_t operation;
        LcdControllers_t ctrl;
        LcdConnectionState_t state;
    } conn;
} lcdQueueElem_t;

typedef struct {
    lcdQueueElem_t lcdQueueElem[LCD_QUEUE_SIZE];
    uint8_t lcdQueueElemNb;
} lcdQueueStruct_t;


extern OS_Q lcdQueue;
extern bool scrSaverStarted;
extern lcdQueueStruct_t lcdQueueStruct;

extern void AppTaskLCDControl(void *p_arg);
extern void send_to_lcd_queue(const lcdQueueElem_t *opQueueElemPtr);
extern lcdQueueElem_t *receive_from_lcd_queue(void);

#endif
