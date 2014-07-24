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
#include "mod_auxiliary.h"

#define BT_INIT_MESSSAGE_TIMEOUT_MAX     100U
#define BT_MESSSAGE_TIMEOUT_MAX          1U

volatile bool uartBTTransmissionDone = false;
volatile bool uartBTMessageReady = false;
BTMessageStruct_t BTMessageStruct = {0};

static void AppTaskBTControlInit(void);
static void AppTaskBTDeviceInit(void);

static void bt_message_transfer_handler(const char *mesg);
static bool bt_message_reveive_wait_ready(uint8_t cnt, uint32_t timeout);

void AppTaskBTControl(void *p_arg)
{
    OS_ERR os_err;
    static uint32_t velocity = 0;
    bool isBTDataFlowActive = false;
    bool isSoundSignalActive = false;

    (void)p_arg;
    AppTaskBTControlInit();
    AppTaskBTDeviceInit();

    while(1) {
        if(bt_message_reveive_wait_ready(1, BT_MESSSAGE_TIMEOUT_MAX) == true) {
            if(strncmp("CONNECT  '64A7-69-8F1B8B'", (char *)BTMessageStruct.BTBuffer, 25U) == 0) {
                printf("\r\nBT Connected\r\n");
                isBTDataFlowActive = true;

                BTMessageStruct.BTBuffer[0] = '\0';
                BTMessageStruct.BTBufferElemNb = 0;
            }
            else if(strncmp("DISCONNECT  '64A7-69-8F1B8B'", (char *)BTMessageStruct.BTBuffer, 28U) == 0) {
                printf("\r\nBT Disconnected\r\n");
                isBTDataFlowActive = false;
                
                BTMessageStruct.BTBuffer[0] = '\0';
                BTMessageStruct.BTBufferElemNb = 0;
            }
            else {
                printf("%s\r\n", BTMessageStruct.BTBuffer);
            }
        }
        else if(isBTDataFlowActive == true) {
            opQueueElem_t opQueueElem = {0};
            uint8_t btByte = BTMessageStruct.BTBuffer[0];
            
            if(BTMessageStruct.BTBufferElemNb == 1 && btByte >= 'a' && btByte <= 'z') {
                switch(btByte) {
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
                        opQueueElem.op = RADIO_LIGHTOP_LEFT;
                        if(lightCurrentState.lightLeftCurrState == RADIO_LIGHTST_DISABLE) {
                            opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                            lightCurrentState.lightLeftCurrState = RADIO_LIGHTST_ENABLE;
                        }
                        else {
                            opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                            lightCurrentState.lightLeftCurrState = RADIO_LIGHTST_DISABLE;
                        }
                        break;
                    case 'p':
                        opQueueElem.funct = RADIO_OP_LIGHTING;
                        opQueueElem.op = RADIO_LIGHTOP_RIGHT;
                        if(lightCurrentState.lightRightCurrState == RADIO_LIGHTST_DISABLE) {
                            opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                            lightCurrentState.lightRightCurrState = RADIO_LIGHTST_ENABLE;
                        }
                        else {
                            opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                            lightCurrentState.lightRightCurrState = RADIO_LIGHTST_DISABLE;
                        }
                        break;
                    case 'o':
                        opQueueElem.funct = RADIO_OP_LIGHTING;
                        opQueueElem.op = RADIO_LIGHTOP_INNER;
                        if(lightCurrentState.lightInnerCurrState == RADIO_LIGHTST_DISABLE) {
                            opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                            lightCurrentState.lightInnerCurrState = RADIO_LIGHTST_ENABLE;
                        }
                        else {
                            opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                            lightCurrentState.lightInnerCurrState = RADIO_LIGHTST_DISABLE;
                        }
                        break;
                    case 'l':
                        opQueueElem.funct = RADIO_OP_LIGHTING;
                        opQueueElem.op = RADIO_LIGHTOP_OUTER;
                        if(lightCurrentState.lightOuterCurrState == RADIO_LIGHTST_DISABLE) {
                            opQueueElem.val_0 = RADIO_LIGHTST_ENABLE;
                            lightCurrentState.lightOuterCurrState = RADIO_LIGHTST_ENABLE;
                        }
                        else {
                            opQueueElem.val_0 = RADIO_LIGHTST_DISABLE;
                            lightCurrentState.lightOuterCurrState = RADIO_LIGHTST_DISABLE;
                        }
                        break;
                    case 'h':
                        if(isSoundSignalActive == false) {
                            isSoundSignalActive = true;
                            bt_message_transfer_handler("Horn on\r");
                        }
                        else {
                            bt_message_transfer_handler("Horn off\r");
                            isSoundSignalActive = false;
                        }
                        break;
                    default: {}
                }

                BTMessageStruct.BTBufferElemNb = 0;
            }
            else if(BTMessageStruct.BTBufferElemNb >= 1 && btByte >= '0' && btByte <= '9') {
                velocity = (btByte - '0') * 10U + 10U;
                BTMessageStruct.BTBufferElemNb = 0;

                printf("velocity: %d\r\n", velocity);
            }
            else if(isSoundSignalActive == true) {
                opQueueElem.funct = RADIO_OP_SOUND_SIG;
                opQueueElem.op = 1;
            }
            
            if(opQueueElem.funct != RADIO_OP_UNDEF) {
                send_to_op_queue(&opQueueElem);
            }
        }
        
        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

void AppTaskBTDeviceInit(void)
{
    OS_ERR os_err;
    bool isBTConnectionOn = false;

    OSTimeDlyHMSM(0, 0, 4, 0, OS_OPT_TIME_PERIODIC, &os_err);

    while(1) {
        bt_message_transfer_handler("\r");
        OSTimeDlyHMSM(0, 0, 1, 200, OS_OPT_TIME_PERIODIC, &os_err);

        bt_message_transfer_handler("+++");

        OSTimeDlyHMSM(0, 0, 1, 200, OS_OPT_TIME_PERIODIC, &os_err);
        bt_message_transfer_handler("\r");

        // first empty response in data mode, wait for the next message
        // wait for two messages from btm222: echo and confirmation
        bt_message_reveive_wait_ready(2, BT_INIT_MESSSAGE_TIMEOUT_MAX);

        if(strncmp("OK", (char *)&BTMessageStruct.BTBuffer, 2U) == 0) {
            printf("\r\nBT connection is on, trying to disconnect\r\n");
            BTMessageStruct.BTBufferElemNb = 0;
            isBTConnectionOn = true;
            break;
        }
        else if (strncmp("+++ERROR", (char *)&BTMessageStruct.BTBuffer, 8U) == 0){
            printf("\r\nBT connection is already off!\r\n");
            BTMessageStruct.BTBufferElemNb = 0;
            break;
        }
        else {
            printf("\r\nSet to command mode unssuported case, retrying!\r\n");
            printf("%s", BTMessageStruct.BTBuffer);
            BTMessageStruct.BTBufferElemNb = 0;
        }
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);
    }

    if(isBTConnectionOn == true) {
        isBTConnectionOn = false;

        while(1) {
            bt_message_transfer_handler("ATH\r");

            //wait for two messages from btm222: echo and confirmation
            bt_message_reveive_wait_ready(2, BT_INIT_MESSSAGE_TIMEOUT_MAX);
            
            if(strncmp("ATH", (char *)&BTMessageStruct.BTBuffer, 3U) == 0  &&
               strncmp("OK", (char *)&BTMessageStruct.BTBuffer[3], 2U) == 0) {
                printf("\r\nPending connection interrupted\r\n");
                BTMessageStruct.BTBufferElemNb = 0;
                break;
            }
            else {
                printf("\r\nCannot interrupt connection, retrying!\r\n");
                printf("%s", BTMessageStruct.BTBuffer);
                BTMessageStruct.BTBufferElemNb = 0;
            }
            OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);
        }
    }

    while(1) {
        bt_message_transfer_handler("ATZ0\r");

        //wait for two messages from btm222: echo and confirmation
        bt_message_reveive_wait_ready(2, BT_INIT_MESSSAGE_TIMEOUT_MAX);
        
        if(strncmp("ATZ0", (char *)&BTMessageStruct.BTBuffer, 4U) == 0  &&
           strncmp("OK", (char *)&BTMessageStruct.BTBuffer[4], 2U) == 0) {
            printf("\r\nDefault settings restored, rebooting\r\n");
            BTMessageStruct.BTBufferElemNb = 0;
            break;
        }
        else {
            printf("\r\nCannot restore default settings, retrying!\r\n");
            printf("%s", BTMessageStruct.BTBuffer);
            BTMessageStruct.BTBufferElemNb = 0;
        }
        OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);
    }

    // there should be delay afret btm222 reseting but retry mechanizm will take care of this
    while(1) {
        bt_message_transfer_handler("ATR1\r");

        //wait for two messages from btm222: echo and confirmation
        bt_message_reveive_wait_ready(2, BT_INIT_MESSSAGE_TIMEOUT_MAX);

        if(strncmp("ATR1", (char *)&BTMessageStruct.BTBuffer, 4U) == 0  &&
           strncmp("OK", (char *)&BTMessageStruct.BTBuffer[4], 2U) == 0) {
            printf("\r\nDevice in slave role\r\n");
            BTMessageStruct.BTBufferElemNb = 0;
            break;
            
        }
        else {
            printf("\r\nCannot set device to slave role, retrying!\r\n");
            printf("%s", BTMessageStruct.BTBuffer);
            BTMessageStruct.BTBufferElemNb = 0;
        }
        
        OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_PERIODIC, &os_err);
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
    static uint8_t prevCharacter = 0;
    uint8_t character = USART_ReceiveData(UART4);
    printf("bt:'%c' ", character);

    if(character >= 0x20) {
        BTMessageStruct.BTBuffer[BTMessageStruct.BTBufferElemNb] = character;

        BTMessageStruct.BTBufferElemNb++;
        //if overflow - neglect the message
        if(BTMessageStruct.BTBufferElemNb >= BT_BUFFER_SIZE_MAX) {
            BTMessageStruct.BTBufferElemNb = 0;
        }

        BTMessageStruct.BTBuffer[BTMessageStruct.BTBufferElemNb] = '\0';
    }
    else if(prevCharacter == '\r' && character == '\n') {
        uartBTMessageReady = true;
    }
    else {
        //unsupported characters
    }

    prevCharacter = character;
}

static void bt_message_transfer_handler(const char *mesg)
{
    uint16_t i = 0;

    uartBTTransmissionDone = false;
    for(i=0; ; i++) {
        USART_SendData(UART4, mesg[i]);
        if(mesg[i+1] != '\0') {
            USART_ClearFlag(UART4, USART_FLAG_TXE);
            while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
        }
        else {
            break;
        }
    }

    while(uartBTTransmissionDone == false);
    uartBTTransmissionDone = false;
}

static bool bt_message_reveive_wait_ready(uint8_t cnt, uint32_t timeout)
{
    OS_ERR os_err;
    bool fresult = true;
    uint16_t messageTimeout = 0;

    while(cnt--) {
        messageTimeout = timeout;
        while(uartBTMessageReady != true) {
            if(messageTimeout--) {
                OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_PERIODIC, &os_err);
            }
            else {
                fresult = false;
                break;
            }
        }

        uartBTMessageReady = false;
    }

    return fresult;
}


/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
