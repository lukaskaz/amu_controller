/****************************************Copyright (c)****************************************************
**                                      
**                                      
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mod_radio_control.c
** Descriptions:            Interface for RF radio operations
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
#include "mod_radio_control.h"
#include <stdbool.h>

#include "mod_lcd.h"
#include "stm32fxxx_it.h"

#define RADIO_USART           USART6
#define RADIO_FRAME_RES_SIZE  2U
#define RADIO_FRAME_SIZE      16U
#define RADIO_BYTES_FOR_CS    (RADIO_FRAME_SIZE - 1U)
#define RADIO_PAYLOAD_SIZE    10U

#define RADIO_UART_ADDRESS    5U

OS_Q opQueue;
OS_TMR rfHeartbeatTimeout;
opQueueStruct_t opQueueStruct = {0};

volatile bool uartRFTransmissionDone = false;
bool rfHeartbeatToSend = false;

typedef enum
{
    RADIO_FRAME_FREE = 0,
    RADIO_FRAME_RECEIVING,
    RADIO_FRAME_READY,
} radioFrameState;

typedef enum
{
    RADIO_FRAME_NORMAL = 0,
    RADIO_FRAME_RESPONSE,
    RADIO_FRAME_OTHER,
} radioFrameType;

typedef enum
{
    RADIO_FUNC_UNDEF = 0,
    RADIO_FUNC_SEND,
    RADIO_FUNC_RECEIVE,
}radioFunctionType;

typedef enum
{
    RADIO_ADDR_BASE = 0,
    RADIO_ADDR_NODE_1,
    RADIO_ADDR_NODE_MAX,
}radio_addresses;

typedef enum
{
    RADIO_RES_TRANSFER_ERROR = 0,
    RADIO_RES_OPERATION_ERROR,
    RADIO_RES_OPERATION_COMPLETE,
    RADIO_RES_OPERATION_UNSUPPORTED,
}radioResponse_t;

typedef union
{
    struct
    {
        radioFrameType  type;
        radioFrameState state;
    } frame; 

    struct
    {
        radioFrameType frameType;
        radioFrameState frameStatus;

        uint8_t byte[RADIO_FRAME_SIZE];
    }iodata;

    struct
    {
        radioFrameType frameType;
        radioFrameState frameStatus;

        uint8_t frame_type;
        uint8_t uart_sender_addr;
        uint8_t function;
        uint8_t radio_target_addr;
        uint8_t payload_bytes_nb;

        union {
            uint8_t function;
            struct {
                uint8_t data[RADIO_PAYLOAD_SIZE];
            } plainData;
            struct {
                uint8_t function;
                uint8_t direction;
                uint8_t speed_0;
                uint8_t speed_1;
            } driveData;
            struct {
                uint8_t function;
                uint8_t lightingType;
                uint8_t lightingState;
            } lightingData;
            struct {
                uint8_t function;
                uint8_t soundSignalStatus;
            } soundSignalData;
        } payload;
        
        uint8_t checksum;
    }uart_payload;

    struct
    {
        radioFrameType frameType;
        radioFrameState frameStatus;

        uint8_t frame_type;
        radioResponse_t res;
    }uart_response;
} radioFrame;

static struct
{
    uint8_t rxFrameBytePos;
    radioFrame txCell;
    radioFrame rxCell;
} radioData = {0};


static void AppTaskRadioControlInit(void);
static bool isRadioRxNormalFrameReady(void);
static bool isRadioRxResponseFrameReady(void);
static void radio_frame_transmit_handler(radioFrame *output);
static bool is_radio_frame_receiving(void);

static uint8_t radio_data_checksum_calculate(const radioFrame *const data);

void rfHeartbeatCallback(void *p_tmr, void *p_arg)
{
    rfHeartbeatToSend = true;
}

void AppTaskRadioControl(void *p_arg)
{
    OS_ERR os_err;
    uint8_t i = 0;
    opQueueElem_t *opQueueElemPtr = NULL;
    radioFrame *rfData = &radioData.txCell;

    AppTaskRadioControlInit();

    rfData->uart_payload.frame_type        = RADIO_FRAME_NORMAL;
    rfData->uart_payload.uart_sender_addr  = RADIO_UART_ADDRESS;
    rfData->uart_payload.function          = RADIO_FUNC_SEND;
    rfData->uart_payload.radio_target_addr = RADIO_ADDR_NODE_1;
    rfData->uart_payload.payload_bytes_nb  = RADIO_PAYLOAD_SIZE;

    OSTmrCreate(&rfHeartbeatTimeout, "Heartbeat timeout", 0, 5, OS_OPT_TMR_PERIODIC, rfHeartbeatCallback, 0, &os_err);
    OSTmrStart((OS_TMR *)&rfHeartbeatTimeout, &os_err);
    while(1) {
        opQueueElemPtr = receive_from_op_queue();

        if(opQueueElemPtr != NULL) {
            OSTmrStart((OS_TMR *)&rfHeartbeatTimeout, &os_err);
            
            for(i=5; i<RADIO_FRAME_SIZE-1; i++) {
                rfData->iodata.byte[i] = 0x00;
            }

            switch(opQueueElemPtr->funct) 
            {
                case RADIO_OP_DRIVE:
                    rfData->uart_payload.payload.function = opQueueElemPtr->funct;
                    rfData->uart_payload.payload.driveData.direction = opQueueElemPtr->op;
                    rfData->uart_payload.payload.driveData.speed_0   = opQueueElemPtr->val_0;
                    rfData->uart_payload.payload.driveData.speed_1   = opQueueElemPtr->val_1;
                    break;
                case RADIO_OP_LIGHTING:
                    rfData->uart_payload.payload.function = opQueueElemPtr->funct;
                    rfData->uart_payload.payload.lightingData.lightingType = opQueueElemPtr->op;
                    rfData->uart_payload.payload.lightingData.lightingState = opQueueElemPtr->val_0;
                    break;
                case RADIO_OP_SOUND_SIG:
                    rfData->uart_payload.payload.function = opQueueElemPtr->funct;
                    rfData->uart_payload.payload.soundSignalData.soundSignalStatus = opQueueElemPtr->op;
                    break;
                default: { }
            }

            radio_frame_transmit_handler(rfData); 
        }
        else {
            if(rfHeartbeatToSend == true) {
                rfHeartbeatToSend = false;

                if(scrSaverStarted == false) {
                    opQueueElem_t opQueueElem   = {0};
                    // heartbeat messages are sustained only when SS is disabled
                    opQueueElem.funct = RADIO_OP_HEARTBEAT;
                    send_to_op_queue(&opQueueElem);
                }
            }
        }

        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

static void AppTaskRadioControlInit(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    USART_InitStructure.USART_BaudRate   = 57600;
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;
    USART_InitStructure.USART_Parity     = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART6, &USART_InitStructure);

    USART_WakeUpConfig(USART6, USART_WakeUp_AddressMark);
    USART_SetAddress(USART6, 0x05);

    USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART6, USART_IT_TC, ENABLE);
    USART_ClearFlag(USART6,USART_FLAG_RXNE);
    USART_ClearFlag(USART6,USART_FLAG_TC);

    BSP_IntVectSet(BSP_INT_ID_USART6, USART6_IRQHandler);
    BSP_IntEn(BSP_INT_ID_USART6);

    USART_Cmd(USART6, ENABLE);
}

static bool isRadioRxNormalFrameReady(void)
{
    bool fresult = false;

    if(radioData.rxCell.frame.state == RADIO_FRAME_READY) {
        radioData.rxCell.frame.state = RADIO_FRAME_FREE;
        fresult = true;
    }

    return fresult;
}


static bool isRadioRxResponseFrameReady(void)
{
    bool fresult = false;

    if(radioData.rxCell.frame.state == RADIO_FRAME_READY && radioData.rxCell.frame.type == RADIO_FRAME_RESPONSE) {
        radioData.rxCell.frame.type  = RADIO_FRAME_NORMAL;
        radioData.rxCell.frame.state = RADIO_FRAME_FREE;
        fresult = true;
    } 
    
    return fresult;
}

void radio_frame_receive_handler(void)
{
    uint8_t data = USART_ReceiveData(RADIO_USART);
    
    if(radioData.rxCell.frame.state == RADIO_FRAME_FREE) {
        //ignore the first received byte (uart target address)
        radioData.rxCell.frame.state = RADIO_FRAME_RECEIVING;
    }
    else {
        radioData.rxCell.iodata.byte[radioData.rxFrameBytePos] = data; 
        radioData.rxFrameBytePos++;

        if( (radioData.rxFrameBytePos == RADIO_FRAME_SIZE && radioData.rxCell.frame.type == RADIO_FRAME_NORMAL)      ||
            (radioData.rxFrameBytePos == RADIO_FRAME_RES_SIZE && radioData.rxCell.frame.type == RADIO_FRAME_RESPONSE) )
        {
            radioData.rxCell.frame.state = RADIO_FRAME_READY;
            radioData.rxFrameBytePos = 0;
        }
    }
    
}

static void radio_frame_transmit_handler(radioFrame *output)
{
    OS_ERR err;
    uint8_t i = 0;
    uint32_t timeout = 0;
    lcdQueueElem_t lcdQueueElem = {0};
    radioFrame *response = &radioData.rxCell;

    output->uart_payload.checksum = radio_data_checksum_calculate(output);
    while(is_radio_frame_receiving() == true);

    // response may come during uart sending, be prepare for that
    response->frame.type = RADIO_FRAME_RESPONSE;

    USART_ClearFlag(RADIO_USART, USART_FLAG_TXE);
    USART_SendData(RADIO_USART, 0x100|0x0A);
    while(USART_GetFlagStatus(RADIO_USART, USART_FLAG_TXE) == RESET);

    for(i=0; i<RADIO_FRAME_SIZE; i++) {
        USART_ClearFlag(RADIO_USART, USART_FLAG_TXE);
        USART_SendData(RADIO_USART, output->iodata.byte[i]);
        while(USART_GetFlagStatus(RADIO_USART, USART_FLAG_TXE) == RESET);
    }

    USART_SendData(RADIO_USART, 0x100|0xFF);
    while(uartRFTransmissionDone == false);
    uartRFTransmissionDone = false;

    timeout = 0;
    while(isRadioRxResponseFrameReady() != true) {
        OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_PERIODIC, &err);
        if(timeout++ >= 1000) {
            printf("Radio response timeout reached!\r\n");
            break;
        }
    }
    //printf("Res %d, %d, %d\n\r", response->uart_response.res, radioData.rxCell.frame.state, radioData.rxCell.frame.type);

    if(response->uart_response.res == RADIO_RES_OPERATION_ERROR) {
            lcdQueueElem.opKind    = LCD_OP_CONN;
            lcdQueueElem.conn.ctrl = LCD_CTRL_NONE;
            lcdQueueElem.conn.state = LCD_CONN_OFFLINE;
    }
    else {
            lcdQueueElem.opKind    = LCD_OP_CONN;
            lcdQueueElem.conn.ctrl = LCD_CTRL_NONE;
            lcdQueueElem.conn.state = LCD_CONN_ONLINE;
    }

    send_to_lcd_queue(&lcdQueueElem);
}

static bool is_radio_frame_receiving(void)
{
    bool fresult = false;
    if(radioData.rxCell.frame.state == RADIO_FRAME_RECEIVING) {
            fresult = true;
    }

    return fresult;
}


static uint8_t radio_data_checksum_calculate(const radioFrame *const data)
{
    uint16_t checksum = 0, i = 0;

    for(i=0; i<RADIO_BYTES_FOR_CS; i++) {
        checksum += data->iodata.byte[i];
    }

    checksum = (uint8_t)((checksum^0xFF) + 1);
    return checksum;
}

void send_to_op_queue(const opQueueElem_t *opQueueNewElemPtr)
{
    static OS_ERR os_err;
    static opQueueElem_t *opQueueDestElemPtr = NULL;

    opQueueDestElemPtr = &opQueueStruct.opQueueElem[opQueueStruct.queueElemNb];
    if(++opQueueStruct.queueElemNb >= OP_QUEUE_SIZE) {
        opQueueStruct.queueElemNb = 0;
    }

    *opQueueDestElemPtr = *opQueueNewElemPtr;
    OSQPost(&opQueue, opQueueDestElemPtr, sizeof(*opQueueDestElemPtr), OS_OPT_POST_FIFO + OS_OPT_POST_ALL, &os_err);
}

opQueueElem_t *receive_from_op_queue(void)
{
    static OS_ERR os_err;
    static OS_MSG_SIZE msg_size;
    
    opQueueElem_t *opQueueElemPtr = OSQPend(&opQueue, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &os_err);
    //opQueueStruct.queueElemNb--;

    return opQueueElemPtr;
}

void flush_op_queue(void)
{
    static OS_ERR os_err;

    OSQFlush(&opQueue, &os_err);
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
