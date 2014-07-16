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

USB_OTG_CORE_HANDLE USB_OTG_dev;


void AppTaskConsole(void *p_arg)
{
    OS_ERR os_err;
    FILE * pFile;
    uint8_t chByte = 0;
    bool showMenu = true;
    static uint32_t velocity = 0;

    extern uint8_t USB_Tx_Buffer[];
    extern uint32_t USB_Tx_ptr_in;
    extern uint32_t USB_Tx_ptr_out;

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

    while(1) {
        opQueueElem_t opQueueElem = {0};

        if(showMenu == true) {
            showMenu = false;

            printf("\n\rPlease insert move direction [w/s/a/d], to adjust speed [v]"
                   "\n\rTo adjust lighting insert [o/l/i/p] to signal press [SPACE]"
                   "\n\rTo show menu press [ESC]\n\r");
        }

        chByte = getc(pFile);
        if(chByte != 0) {
            switch(chByte) 
            {
                case 0x1B:
                    showMenu = true;
                    break;
                case 'w':
                    opQueueElem.funct = RADIO_OP_DRIVE;
                    opQueueElem.op    = RADIO_DRVOP_FORWARD;
                    opQueueElem.val_0 = velocity;
                    opQueueElem.val_1 = velocity;
                    break;
                case 's':
                    opQueueElem.funct = RADIO_OP_DRIVE;
                    opQueueElem.op = RADIO_DRVOP_BACKWARD;
                    opQueueElem.val_0 = velocity;
                    opQueueElem.val_1 = velocity;
                    break;
                case 'a':
                    opQueueElem.funct = RADIO_OP_DRIVE;
                    opQueueElem.op = RADIO_DRVOP_LEFT;
                    opQueueElem.val_0 = velocity;
                    break;
                case 'd':
                    opQueueElem.funct = RADIO_OP_DRIVE;
                    opQueueElem.op = RADIO_DRVOP_RIGHT;
                    opQueueElem.val_1 = velocity;
                    break;
                case 'v':
                    printf("\nPlease insert velocity value [0 to 100]\n\r");
                    scanf("%d", &velocity);
                    printf("\nVelocity set to %d%%\n\r", velocity);
                    break;
                case 'i':
                    opQueueElem.funct = RADIO_OP_LIGHTING;
                    opQueueElem.op = 1;
                    break;
                case 'p':
                    opQueueElem.funct = RADIO_OP_LIGHTING;
                    opQueueElem.op = 2;
                    break;
                case 'o':
                    opQueueElem.funct = RADIO_OP_LIGHTING;
                    opQueueElem.op = 3;
                    break;
                case 'l':
                    opQueueElem.funct = RADIO_OP_LIGHTING;
                    opQueueElem.op = 4;
                    break;
                case ' ':
                    opQueueElem.funct = RADIO_OP_SOUND_SIG;
                    opQueueElem.op = 1;
                    break;
                default: { }
            }
        }
        
        if(opQueueElem.funct != RADIO_OP_UNDEF) {
            send_to_op_queue(&opQueueElem);
        }

        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);
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
        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &err);
    }
    ch = USB_Rx_Buffer[chNb++];

    if(chNb >= USB_Rx_length) {
        chNb = 0;
        USB_Rx_length = 0;
    }

    return ch;
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
