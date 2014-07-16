/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2003-2012; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                           STM3220G-EVAL
*                                         Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : EHS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <includes.h>
#include "mod_console.h"
#include "mod_joystick.h"
#include "mod_bt_control.h"
#include "mod_radio_control.h"
#include "mod_lcd.h"

#include "stm32fxxx_it.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* ----------------- APPLICATION GLOBALS ------------------ */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB1;
static  CPU_STK  AppTaskStartStk1[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB2;
static  CPU_STK  AppTaskStartStk2[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB3;
static  CPU_STK  AppTaskStartStk3[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB4;
static  CPU_STK  AppTaskStartStk4[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB5;
static  CPU_STK  AppTaskStartStk5[APP_TASK_START_STK_SIZE];
static  OS_TCB   AppTaskStartTCB6;
static  CPU_STK  AppTaskStartStk6[APP_TASK_START_STK_SIZE];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);

static void AppTaskCreate(void);
static void AppObjCreate(void);

static void AppTaskLedBlinking(void *p_arg);
static void AppTaskLedBlinkingInit(void);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;

    BSP_IntDisAll();                                            /* Disable all interrupts.                                  */

    CPU_Init();                                                 /* Initialize uC/CPU services.                              */

    OSInit(&err);                                               /* Init OS.                                                 */

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                /* Create the start task                                    */
                 (CPU_CHAR   *)"App Task Start",
                 (OS_TASK_PTR ) AppTaskStart,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO,
                 (CPU_STK    *)&AppTaskStartStk[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSStart(&err);

    while (DEF_ON) {                                            /* Should Never Get Here                                    */
    };
}


/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR os_err;
    (void)p_arg;

    BSP_Init();                                                 /* Init BSP fncts.                                          */

    CPU_Init();                                                 /* Init CPU name & int. dis. time measuring fncts.          */

    Mem_Init();                                                 /* Initialize Memory managment                              */

    BSP_CPU_TickInit();                                         /* Start Tick Initialization                                */

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();                                               /* Determine CPU capacity                                   */
#endif

#if (APP_CFG_SERIAL_EN == DEF_ENABLED)
    BSP_Ser_Init(115200);                                      /* Initialize Serial Interface                              */
#endif

    APP_TRACE_INFO(("Creating Application Objects... \n\r"));
    AppObjCreate();                                             /* Create Application Kernel Objects                        */

    APP_TRACE_INFO(("Creating Application Tasks... \n\r"));
    AppTaskCreate();                                            /* Create Application Tasks                                 */

    //System intitialisation comlpete, start task is to be ended
    OSTaskDel(&AppTaskStartTCB, &os_err);

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.           */
        //BSP_LED_Toggle(0);
        //OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_PERIODIC, &os_err);
    }
}


/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
    OS_ERR      os_err;

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB1,
                 (CPU_CHAR   *)"App Task Led Blinking",
                 (OS_TASK_PTR ) AppTaskLedBlinking,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 1U,
                 (CPU_STK    *)&AppTaskStartStk1[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB2,
                 (CPU_CHAR   *)"App Task Radio Control",
                 (OS_TASK_PTR ) AppTaskRadioControl,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 3U,
                 (CPU_STK    *)&AppTaskStartStk2[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB3,
                 (CPU_CHAR   *)"App Task Console",
                 (OS_TASK_PTR ) AppTaskConsole,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 1U,
                 (CPU_STK    *)&AppTaskStartStk3[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB4,
                 (CPU_CHAR   *)"App Task Joystick Control",
                 (OS_TASK_PTR ) AppTaskJoyControl,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 2U,
                 (CPU_STK    *)&AppTaskStartStk4[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
                 
    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB5,
                 (CPU_CHAR   *)"App Task BT Control",
                 (OS_TASK_PTR ) AppTaskBTControl,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 2U,
                 (CPU_STK    *)&AppTaskStartStk5[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB6,
                 (CPU_CHAR   *)"App Task LCD",
                 (OS_TASK_PTR ) AppTaskLCDControl,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO + 4U,
                 (CPU_STK    *)&AppTaskStartStk6[0],
                 (CPU_STK     )(APP_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
}


/*
*********************************************************************************************************
*                                          AppObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void  AppObjCreate (void)
{
    OS_ERR os_err;
    OSQCreate(&opQueue, "Console Operations Queue", OP_QUEUE_SIZE, &os_err);
    OSQCreate(&lcdQueue, "Console LCD Queue", LCD_QUEUE_SIZE, &os_err);
}

static void AppTaskLedBlinking(void *p_arg)
{
    OS_ERR os_err;
    (void)p_arg;
    
    AppTaskLedBlinkingInit();
    while(1) {
        GPIO_ToggleBits(GPIOF, GPIO_Pin_6);
        OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_PERIODIC, &os_err);
    }
}

static void AppTaskLedBlinkingInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct; 

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);

    GPIO_InitStruct.GPIO_Pin=GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_UP;
    GPIO_Init(GPIOF,&GPIO_InitStruct);

}



