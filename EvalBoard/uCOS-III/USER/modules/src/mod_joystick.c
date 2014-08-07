/****************************************Copyright (c)****************************************************
**                                      
**                                      
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mod_joystick.c
** Descriptions:            Interface for joystick operations
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
#include "mod_joystick.h"
#include <stdbool.h>
#include <includes.h>

#include "mod_radio_control.h"
#include "mod_lcd.h"
#include "mod_auxiliary.h"


#define ADC1_DR_ADDRESS          ((uint32_t)0x4001204C)

#define MAX_ADC_RES_VALUE        4000U
#define HALF_ADC_RES_VALUE       2000U


static void AppTaskJoyControlInit(void);


__IO uint16_t ADCConvertedValue[2] = {0};

void AppTaskJoyControl(void *p_arg)
{
    OS_ERR os_err;
    (void)p_arg;

    AppTaskJoyControlInit();
    OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_PERIODIC, &os_err);

    while(1) {
        //printf("ADC: %d | %d\n\r", ADCConvertedValue[0], ADCConvertedValue[1]);
        opQueueElem_t opQueueElem   = {0};
        lcdQueueElem_t lcdQueueElem = {0};
        uint16_t vertMovementFactor = 100U*ADCConvertedValue[0]/MAX_ADC_RES_VALUE;
        uint16_t horiMovementFactor = 100U*ADCConvertedValue[1]/MAX_ADC_RES_VALUE;

        opQueueElem.ctrl = RADIO_CTRL_JOYSTICK;
        if(vertMovementFactor < 45U) {
            uint8_t speedFwd = MAX_VELOCITY_VALUE - (100U * ADCConvertedValue[0] / HALF_ADC_RES_VALUE);

            opQueueElem.funct = RADIO_OP_DRIVE;
            opQueueElem.op    = RADIO_DRVOP_JOY_FORWARD;
            opQueueElem.val_0 = speedFwd;
            opQueueElem.val_1 = speedFwd;

            lcdQueueElem.opKind    = LCD_OP_MOVE;
            lcdQueueElem.move.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.move.dir  = LCD_MV_FORWARD;
            lcdQueueElem.move.speed = speedFwd;
        }
        else if(vertMovementFactor > 50U) {
            uint8_t speedBwd = (100U * ADCConvertedValue[0] / HALF_ADC_RES_VALUE) - MAX_VELOCITY_VALUE;

            opQueueElem.funct = RADIO_OP_DRIVE;
            opQueueElem.op    = RADIO_DRVOP_JOY_BACKWARD;
            opQueueElem.val_0 = speedBwd;
            opQueueElem.val_1 = speedBwd;

            lcdQueueElem.opKind    = LCD_OP_MOVE;
            lcdQueueElem.move.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.move.dir = LCD_MV_BACKWARD;
            lcdQueueElem.move.speed = speedBwd;
        }

        if(horiMovementFactor < 45U) {
            if(opQueueElem.funct == RADIO_OP_UNDEF) {
                opQueueElem.funct = RADIO_OP_DRIVE;
                opQueueElem.op    = RADIO_DRVOP_JOY_RIGHT;
            }

            opQueueElem.val_0 = 0;
            opQueueElem.val_1 = MAX_VELOCITY_VALUE;

            lcdQueueElem.opKind    = LCD_OP_MOVE;
            lcdQueueElem.move.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.move.dir = LCD_MV_RIGHT;
            lcdQueueElem.move.speed = MAX_VELOCITY_VALUE;
        }
        else if(horiMovementFactor > 50U ) {
            if(opQueueElem.funct == RADIO_OP_UNDEF) {
                opQueueElem.funct = RADIO_OP_DRIVE;
                opQueueElem.op    = RADIO_DRVOP_JOY_LEFT;
            }

            opQueueElem.val_0 = MAX_VELOCITY_VALUE;
            opQueueElem.val_1 = 0;

            lcdQueueElem.opKind    = LCD_OP_MOVE;
            lcdQueueElem.move.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.move.dir = LCD_MV_LEFT;
            lcdQueueElem.move.speed = MAX_VELOCITY_VALUE;
        }

        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3) == Bit_RESET) {
            opQueueElem.funct = RADIO_OP_SOUND_SIG;
            opQueueElem.op = RADIO_SIGOP_ON;

            lcdQueueElem.opKind    = LCD_OP_HORN;
            lcdQueueElem.horn.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.horn.state = LCD_HRN_ON;
        }
        else if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == Bit_RESET) {
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == Bit_RESET) {
                OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_PERIODIC, &os_err);
            }

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

            lcdQueueElem.opKind     = LCD_OP_LIGHT;
            lcdQueueElem.light.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.light.type = LCD_LIG_LEFT;
            lcdQueueElem.light.state = opQueueElem.val_0;
        }
        else if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == Bit_RESET) {
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == Bit_RESET) {
                OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_PERIODIC, &os_err);
            }

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
            
            lcdQueueElem.opKind     = LCD_OP_LIGHT;
            lcdQueueElem.light.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.light.type = LCD_LIG_RIGHT;
            lcdQueueElem.light.state = opQueueElem.val_0;
        }
        else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == Bit_RESET) {
            while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == Bit_RESET) {
                OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_PERIODIC, &os_err);
            }

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

            lcdQueueElem.opKind     = LCD_OP_LIGHT;
            lcdQueueElem.light.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.light.type = LCD_LIG_INNER;
            lcdQueueElem.light.state = opQueueElem.val_0;
        }
        else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == Bit_RESET) {
            while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == Bit_RESET) {
                OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_PERIODIC, &os_err);
            }
            
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

            lcdQueueElem.opKind     = LCD_OP_LIGHT;
            lcdQueueElem.light.ctrl = LCD_CTRL_JOYSTICK;
            lcdQueueElem.light.type = LCD_LIG_OUTER;
            lcdQueueElem.light.state = opQueueElem.val_0;
        }
        else {
            // no action defined
        }

        if(opQueueElem.funct != RADIO_OP_UNDEF) {
            send_to_op_queue(&opQueueElem);
        }
        if(lcdQueueElem.opKind != LCD_OP_NONE) {
            send_to_lcd_queue(&lcdQueueElem);
        }

        //printf("testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest ");
        OSTimeDlyHMSM(0, 0, 0, 20, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

static void AppTaskJoyControlInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct; 
    DMA_InitTypeDef       DMA_InitStructure;
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN; 
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN; 
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN; 
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN; 
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN; 
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    DMA_DeInit(DMA2_Stream0);
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_ADDRESS;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValue;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = 2;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;         
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
    /* DMA2_Stream0 enable */
    DMA_Cmd(DMA2_Stream0, ENABLE);

    ADC_DeInit();
    /* ADC Common Init */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; 
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC1 Init */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    //ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 2;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channel4 & channel6 configuration */ 
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_112Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 2, ADC_SampleTime_112Cycles);

    /* Enable DMA request after last transfer (Single-ADC mode) */
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

    /* Enable ADC1 DMA */
    ADC_DMACmd(ADC1, ENABLE); 

    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* Start ADC1 Software Conversion */ 
    ADC_SoftwareStartConv(ADC1);
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
