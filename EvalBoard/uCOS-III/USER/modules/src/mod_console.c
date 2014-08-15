/****************************************Copyright (c)****************************************************
**                                      
**                                      
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mod_console.c
** Descriptions:            Interface for console operations
**
**--------------------------------------------------------------------------------------------------------
** Created by:              
** Created date:            
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/
#include "mod_console.h"

#include <stdbool.h>

#include "mod_radio_control.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"

#include "stm32fxxx_it.h"
#include "mod_auxiliary.h"

USB_OTG_CORE_HANDLE USB_OTG_dev;

typedef enum {
    CON_OP_NONE        = 0,
    CON_OP_MENU_SHOW   = 0x1B,
    CON_OP_MV_FORWARD  = 'w',
    CON_OP_MV_BACKWARD = 's',
    CON_OP_MV_LEFT     = 'a',
    CON_OP_MV_RIGHT    = 'd',
    CON_OP_SPD_UP      = 'q',
    CON_OP_SPD_DOWN    = 'e',
    CON_OP_SPD_INSERT  = 'v',
    CON_OP_SD_HORN     = ' ',
    CON_OP_LGT_LEFT    = 'i',
    CON_OP_LGT_RIGHT   = 'p',
    CON_OP_LGT_INNER   = 'o',
    CON_OP_LGT_OUTER   = 'l',
} consoleOperation_t;

void fgetcRelease(void);

void AppTaskConsole(void *p_arg)
{
    OS_ERR os_err;
    bool showMenu = true;
    static int32_t velocity = 0;

    extern uint8_t USB_Tx_Buffer[];
    extern uint32_t USB_Tx_ptr_in;
    extern uint32_t USB_Tx_ptr_out;

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

    while(1) {
        consoleOperation_t consoleOperation = CON_OP_NONE;
        opQueueElem_t opQueueElem = {0};

        if(showMenu == true) {
            showMenu = false;

            printf("\n\r>Please insert move direction [w/s/a/d]"
                   "\n\rTo adjust speed [v] -> increase[q], decrease[e]"
                   "\n\rTo adjust lighting insert [o/l/i/p] to signal press [SPACE]"
                   "\n\rTo show menu press [ESC]\n\r");
        }

        consoleOperation = (consoleOperation_t)getchar();
        fgetcRelease();

        opQueueElem.ctrl = RADIO_CTRL_CONSOLE;
        switch(consoleOperation) 
        {
            case 0x1B:
                showMenu = true;
                break;
            case CON_OP_MV_FORWARD:
                opQueueElem.operation = RADIO_OP_DRIVE;
                opQueueElem.opAction  = RADIO_DRVOP_FORWARD;
                opQueueElem.val_0 = velocity;
                opQueueElem.val_1 = velocity;
                break;
            case CON_OP_MV_BACKWARD:
                opQueueElem.operation = RADIO_OP_DRIVE;
                opQueueElem.opAction  = RADIO_DRVOP_BACKWARD;
                opQueueElem.val_0 = velocity;
                opQueueElem.val_1 = velocity;
                break;
            case CON_OP_MV_LEFT:
                opQueueElem.operation = RADIO_OP_DRIVE;
                opQueueElem.opAction  = RADIO_DRVOP_LEFT;
                opQueueElem.val_0 = MAX_VELOCITY_VALUE;
                opQueueElem.val_1 = 0;
                break;
            case CON_OP_MV_RIGHT:
                opQueueElem.operation = RADIO_OP_DRIVE;
                opQueueElem.opAction  = RADIO_DRVOP_RIGHT;
                opQueueElem.val_0 = 0;
                opQueueElem.val_1 = MAX_VELOCITY_VALUE;
                break;
            case CON_OP_SPD_UP:
                velocity += 10U;
                if(velocity > MAX_VELOCITY_VALUE) {
                    velocity = MAX_VELOCITY_VALUE;
                }
                printf("\nVelocity increased to %d%%\n\r", velocity);
                break;
            case CON_OP_SPD_DOWN:
                velocity -= 10U;
                if(velocity < 0) {
                    velocity = 0;
                }
                printf("\nVelocity decreased to %d%%\n\r", velocity);
                break;
            case CON_OP_SPD_INSERT:
                printf("\nPlease insert velocity value [0 to 100]\n\r");
                scanf("%d", &velocity);
                if(velocity > MAX_VELOCITY_VALUE) {
                    velocity = MAX_VELOCITY_VALUE;
                }
                else if(velocity < 0) {
                    velocity = 0;
                }
                else {
                    // velocity within acceptable limits, do nothing
                }
                printf("\nVelocity set to %d%%\n\r", velocity);
                break;
            case CON_OP_LGT_LEFT:
                opQueueElem.operation = RADIO_OP_LIGHTING;
                opQueueElem.opAction  = RADIO_LIGHTOP_LEFT;
                if(lightCurrentState.lightLeftCurrState == RADIO_LIGHTST_DISABLE) {
                    opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                    lightCurrentState.lightLeftCurrState = RADIO_LIGHTST_ENABLE;
                }
                else {
                    opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                    lightCurrentState.lightLeftCurrState = RADIO_LIGHTST_DISABLE;
                }
                break;
            case CON_OP_LGT_RIGHT:
                opQueueElem.operation = RADIO_OP_LIGHTING;
                opQueueElem.opAction  = RADIO_LIGHTOP_RIGHT;
                if(lightCurrentState.lightRightCurrState == RADIO_LIGHTST_DISABLE) {
                    opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                    lightCurrentState.lightRightCurrState = RADIO_LIGHTST_ENABLE;
                }
                else {
                    opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                    lightCurrentState.lightRightCurrState = RADIO_LIGHTST_DISABLE;
                }
                break;
            case CON_OP_LGT_INNER:
                opQueueElem.operation = RADIO_OP_LIGHTING;
                opQueueElem.opAction  = RADIO_LIGHTOP_INNER;
                if(lightCurrentState.lightInnerCurrState == RADIO_LIGHTST_DISABLE) {
                    opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                    lightCurrentState.lightInnerCurrState = RADIO_LIGHTST_ENABLE;
                }
                else {
                    opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                    lightCurrentState.lightInnerCurrState = RADIO_LIGHTST_DISABLE;
                }
                break;
            case CON_OP_LGT_OUTER:
                opQueueElem.operation = RADIO_OP_LIGHTING;
                opQueueElem.opAction  = RADIO_LIGHTOP_OUTER;
                if(lightCurrentState.lightOuterCurrState == RADIO_LIGHTST_DISABLE) {
                    opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                    lightCurrentState.lightOuterCurrState = RADIO_LIGHTST_ENABLE;
                }
                else {
                    opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                    lightCurrentState.lightOuterCurrState = RADIO_LIGHTST_DISABLE;
                }
                break;
            case CON_OP_SD_HORN:
                opQueueElem.operation = RADIO_OP_SOUND_SIG;
                opQueueElem.opAction  = RADIO_SIGOP_ON;
                break;
            default: { }
        }

        send_to_radio_queue(&opQueueElem);

        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

/*
    I/O Retargeting routines
*/
int fputc(int ch, FILE *f)
{
    extern uint8_t USB_Tx_Buffer[];
    extern uint32_t USB_Tx_ptr_in;

    USB_Tx_Buffer[USB_Tx_ptr_in] = (uint8_t)ch;
    USB_Tx_ptr_in++;

    if(USB_Tx_ptr_in >= USB_TX_DATA_SIZE)
    {
        USB_Tx_ptr_in = 0; 
    }

    return ch;
}

int fgetc(FILE *f) {
    extern uint8_t USB_Rx_Buffer[];
    extern volatile uint32_t USB_Rx_length;
    static uint32_t chNb = 0;
    char ch = 0;
    OS_ERR err;

    while(USB_Rx_length == 0) {
        chNb = 0;
        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &err);
    }
    ch = USB_Rx_Buffer[chNb++];

    if(chNb >= USB_Rx_length) {
        USB_Rx_length = 0;
    }

    return ch;
}

void fgetcRelease(void)
{
    extern volatile uint32_t USB_Rx_length;

    USB_Rx_length = 0;
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
