/****************************************Copyright (c)****************************************************
**                                      
**                                      
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mod_bt_control.c
** Descriptions:            Interface for bluetooth operations
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
#include "mod_bt_control.h"
#include <stdbool.h>
#include <string.h>

#include "mod_radio_control.h"
#include "stm32fxxx_it.h"

volatile bool uartBTTransmissionDone = false;
volatile bool uartBTMessageReady = false;
BTMessageStruct_t BTMessageStruct = {0};

static void AppTaskBTControlInit(void);
static void AppTaskBTDeviceInit(void);


void AppTaskBTControl(void *p_arg)
{
    OS_ERR os_err;
    uint8_t btByte = 0;
    static uint32_t velocity = 0;
    bool isBTDataFlowActive = false;
    bool isSoundSignalActive = false;

    (void)p_arg;
    AppTaskBTControlInit();
    AppTaskBTDeviceInit();

    while(1) {
        if(uartBTMessageReady == true) {
            uartBTMessageReady = false;
            
            if(strncmp("CONNECT", (char *)BTMessageStruct.BTBuffer, 7U) == 0) {
                printf("\r\nBT Connected\r\n");
                isBTDataFlowActive = true;
            }
            else if(strncmp("DISCONNECT", (char *)BTMessageStruct.BTBuffer, 10U) == 0) {
                printf("\r\nBT Disconnected\r\n");
                isBTDataFlowActive = false;
            }

            //printf("%s", BTMessageStruct.BTBuffer);
            BTMessageStruct.BTBufferElemNb = 0;
        }
        else if( isBTDataFlowActive == true &&(BTMessageStruct.BTBufferElemNb != 0 || 
                 isSoundSignalActive == true) )
        {
            opQueueElem_t opQueueElem = {0};

            btByte = BTMessageStruct.BTBuffer[0];
            switch(btByte) 
            {
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
                    if(isSoundSignalActive == false) {
                        isSoundSignalActive = true;
                    }
                    else {
                        isSoundSignalActive = false;
                    }
                    BTMessageStruct.BTBuffer[0] = '\0';

                    break;
                default: { }
            }

            if(isSoundSignalActive == true) {
                opQueueElem.funct = RADIO_OP_SOUND_SIG;
                opQueueElem.op = 1;
            }

            if(BTMessageStruct.BTBuffer[0] >= '0' && BTMessageStruct.BTBuffer[0] <= '9') {
                velocity = (BTMessageStruct.BTBuffer[0] - '0') * 10U + 10U;
                //printf("velocity: %d\r\n", velocity);
            }

            if(opQueueElem.funct != RADIO_OP_UNDEF) {
                send_to_op_queue(&opQueueElem);
            }

            //printf("%s", BTMessageStruct.BTBuffer);
            BTMessageStruct.BTBufferElemNb = 0;
        }

        OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

void AppTaskBTDeviceInit(void)
{
    OS_ERR os_err;
    uint8_t i = 0;
    uint8_t cmdSetToSlaveRole[] = "\rATR1\r";
    
    while(1) {
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_PERIODIC, &os_err);
        for(i=0; cmdSetToSlaveRole[i] != '\0'; i++) {
            USART_SendData(UART4, cmdSetToSlaveRole[i]);
            while(uartBTTransmissionDone == false);
            uartBTTransmissionDone = false;
        }

        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);
        if(uartBTMessageReady == true) {
            uartBTMessageReady = false;
            
            if(strncmp("ATR1", (char *)&BTMessageStruct.BTBuffer, 4U) == 0  &&
               strncmp("OK", (char *)&BTMessageStruct.BTBuffer[4], 2U) == 0) {
                printf("\r\nInitialized!!\r\n");
                BTMessageStruct.BTBufferElemNb = 0;
                break;
                
            }
            else {
                printf("\r\nError!!\r\n");
                //printf("%s", BTMessageStruct.BTBuffer);
                BTMessageStruct.BTBufferElemNb = 0;
            }
        }
    }
    
}

static void AppTaskBTControlInit(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);

    USART_InitStructure.USART_BaudRate   = 19200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;
    USART_InitStructure.USART_Parity     = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART4, &USART_InitStructure);

    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_ITConfig(UART4, USART_IT_TC, ENABLE);
    USART_ClearFlag(UART4,USART_FLAG_RXNE);
    USART_ClearFlag(UART4,USART_FLAG_TC);

    BSP_IntVectSet(BSP_INT_ID_USART4, UART4_IRQHandler);
    BSP_IntPrioSet(BSP_INT_ID_USART4, 4);
    BSP_IntEn(BSP_INT_ID_USART4);

    USART_Cmd(UART4, ENABLE);
}

void bt_message_receive_handler(void)
{
    uint8_t character = USART_ReceiveData(UART4);

    if(character == '\r' || character == '\n') {
        uartBTMessageReady = true;
    }
    else {
        BTMessageStruct.BTBuffer[BTMessageStruct.BTBufferElemNb] = character;

        BTMessageStruct.BTBufferElemNb++;
        //if overflow - neglect the message
        if(BTMessageStruct.BTBufferElemNb >= BT_BUFFER_SIZE_MAX) {
            BTMessageStruct.BTBufferElemNb = 0;
        }
        
        BTMessageStruct.BTBuffer[BTMessageStruct.BTBufferElemNb] = '\0';
    }
}


/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
