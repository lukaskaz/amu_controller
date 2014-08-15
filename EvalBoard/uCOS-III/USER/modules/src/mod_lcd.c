/****************************************Copyright (c)****************************************************
**                                      
**                                      
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mod_lcd.c
** Descriptions:            Interface for controlling lcd screen
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
#include "mod_lcd.h"
#include "mod_lcd_consts.h"
#include <string.h>

//this value should be kept as odd
#define CONNECTION_LOCK_CNT_MAX   7U

static void AppTaskLCDInit(void);
static LcdLightState_t *LCDLastLightingData(const lcdQueueElem_t *data);
static LcdConnectionState_t LCDLastConnectionData(const lcdQueueElem_t *data);
static void SetBit(int bits);
static void ClrBit(int bits);
static void WriteSpi(unsigned char data);
static void WriteSpiCommand(unsigned char cmd);
static void WriteSpiData(unsigned char data);
static void SetContrast(unsigned char contr);
static void LCDClearScreen(LCD_color color);
static void LCDSetPixel(int x, int y, unsigned int color);
static void LCDBox(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char fill, LCD_color color);
static void LCDLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, int color);
static void LCDDrawImage(const unsigned int image[132][132]);
static void LCDPutChar(char c, int  x, int  y, LCD_font_size size, LCD_color fColor, LCD_color bColor);
static void LCDPutStr(char *pString, int  x, int  y, LCD_font_size Size, LCD_color fColor, LCD_color bColor);

OS_Q lcdQueue;
OS_TMR lcdRefreshTimeout;
OS_TMR lcdScrSaverTimeout;
OS_TMR lcdConnLockTimeout;
lcdQueueStruct_t lcdQueueStruct = {0};

bool endActionRefresh = false;
bool scrSaverStarted = false;
bool startScrSaver   = false;
bool startConnLock   = false;
bool connLockStarted = false;

const char *controllerTbl[] = { "None    ", "Console ", "Joystick", "Blutooth" };
const char *directionTbl[]  = { "Stopped ", "Forward ", "Backward", "Left    ", "Right   " };
const char *lightingTbl[]   = { "Off  ", "Left ", "Right", "Inner", "Outer" };
const char *soundSigTbl[]   = { "Off", "On " };
const char *connectionTbl[] = { "OFFLINE", "ONLINE ", "       " };

typedef struct {
    LcdControllers_t contr;
    LcdMoveDirection_t dir;
    uint8_t speed;
    LcdLightState_t light;
    LcdHornState_t horn;
} LCDFieldsVal_t;

typedef struct {
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} LcdLightingLayoutVal_t;

void LCDGetActualLightingLayout(LcdLightingLayoutVal_t *layout)
{
    LcdLightState_t *actualLightDataPtr = LCDLastLightingData(NULL);

    layout->left = 129;
    layout->right = 129;
    layout->middle = 128;

    if( actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_ENABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE ) {
        layout->left = 133;
    }
    if( actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_ENABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE ) {
        layout->left = 132;
    }
    if( actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_DISABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE ) {
        layout->left = 131;
    }
    if( actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_ENABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE ) {
        layout->right = 133;
    }
    if( actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_ENABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE ) {
        layout->right = 132;
    }
    if( actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_DISABLE &&
        actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE ) {
        layout->right = 131;
    }
    if( actualLightDataPtr[LCD_LIG_INNER] == LCD_LIG_ENABLE) {
        layout->middle = 130;
    }
}

void LCDUpdateLightingLayout(const lcdQueueElem_t *data, LcdLightingLayoutVal_t *layout)
{
    LcdLightState_t *actualLightDataPtr = LCDLastLightingData(NULL);
        
    if( data->light.type == LCD_LIG_INNER) {
        if(data->light.state == LCD_LIG_ENABLE) {
            layout->middle = 130;
        }
        else {
            layout->middle = 128;
        }
    }
    if(data->light.type == LCD_LIG_LEFT) {
        if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE) {
            layout->left = 133;
        }
        else if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE) {
            layout->left = 132;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE) {
            layout->left = 131;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE) {
            layout->left = 129;
        }
    }
    else if(data->light.type == LCD_LIG_RIGHT) {
        if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE) {
            layout->right = 133;
        }
        else if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE) {
            layout->right = 132;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_ENABLE) {
            layout->right = 131;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_OUTER] == LCD_LIG_DISABLE) {
           layout->right = 129;
        }
    }
    else if(data->light.type == LCD_LIG_OUTER) {
        if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_ENABLE) {
            layout->left = 133;
        }
        else if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_DISABLE) {
            layout->left = 131;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_ENABLE) {
            layout->left = 132;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_LEFT] == LCD_LIG_DISABLE) {
            layout->left = 129;
        }
        
        if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_ENABLE) {
            layout->right = 133;
        }
        else if(data->light.state == LCD_LIG_ENABLE && actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_DISABLE) {
            layout->right = 131;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_ENABLE) {
            layout->right = 132;
        }
        else if(data->light.state == LCD_LIG_DISABLE && actualLightDataPtr[LCD_LIG_RIGHT] == LCD_LIG_DISABLE) {
            layout->right = 129;
        }
    }
}

void LCDRefreshCallback(void *p_tmr, void *p_arg)
{
    endActionRefresh = true;
}

void LCDScrSaverCallback(void *p_tmr, void *p_arg)
{
    startScrSaver = true;
}

void LCDConnLockCallback(void *p_tmr, void *p_arg)
{
    startConnLock = true;
}

void LCDDisplayData(lcdQueueElem_t *data, bool refresh)
{
    char line[20+1];

    switch(data->operation) {
        case LCD_OP_MOVE:
            snprintf(line, 20, "Ctrll: %s", controllerTbl[data->move.ctrl]);
            LCDPutStr(line, 10, 60, MEDIUM, BLACK, WHITE);
            snprintf(line, 20, "Direc: %s", directionTbl[data->move.dir]);
            LCDPutStr(line, 10, 40, MEDIUM, BLACK, WHITE);
            snprintf(line, 20, "Speed: %03d%%", data->move.speed);
            LCDPutStr(line, 10, 30, MEDIUM, BLACK, WHITE);
            break;
        case LCD_OP_LIGHT: {
            LcdLightingLayoutVal_t layout = {0};
            
            LCDGetActualLightingLayout(&layout);
            if(refresh == 0) {
                LCDUpdateLightingLayout(data, &layout);
            }

            snprintf(line, 20, "Ctrll: %s", controllerTbl[data->light.ctrl]);
            LCDPutStr(line, 10, 60, MEDIUM, BLACK, WHITE);
            snprintf(line, 20, "Light: %c%c%c%c", layout.left, layout.middle, layout.middle, layout.right);
            LCDPutStr(line, 10, 20, MEDIUM, BLACK, WHITE);
            break;
        }
        case LCD_OP_HORN:
            snprintf(line, 20, "Ctrll: %s", controllerTbl[data->horn.ctrl]);
            LCDPutStr(line, 10, 60, MEDIUM, BLACK, WHITE);
            snprintf(line, 20, "Horn:  %s", soundSigTbl[data->horn.state]);
            LCDPutStr(line, 10, 10, MEDIUM, BLACK, WHITE);
            break;
        case LCD_OP_CONN:
            if(refresh == 1) {
                data->conn.state = LCDLastConnectionData(NULL);
            }
            
            //snprintf(line, 20, "Ctrll: %s", controllerTbl[data->conn.ctrl]);
            //LCDPutStr(line, 10, 60, MEDIUM, BLACK, WHITE);
            snprintf(line, 20, "REMOTE %s", connectionTbl[data->conn.state]);
            LCDPutStr(line, 10, 80, MEDIUM, BLACK, WHITE);
            break;
        default: {}
    }

}

void LCDClearDataFields(void)
{
    lcdQueueElem_t data = {0};

    data.operation = LCD_OP_MOVE;
    LCDDisplayData(&data, 1);
    
    data.operation = LCD_OP_HORN;
    LCDDisplayData(&data, 1);
    
    data.operation = LCD_OP_LIGHT;
    LCDDisplayData(&data, 1);
    
    data.operation = LCD_OP_CONN;
    LCDDisplayData(&data, 1);
}


void LCDRefreshScreen(void)
{
    LCDClearScreen(WHITE);

    LCDPutStr("AMUv2 controller", 5, 110, LARGE, BLACK, WHITE);
    LCDLine(10, 99, 132-10, 99, BLACK);
    LCDLine(10, 100, 132-10, 100, BLACK);

    LCDClearDataFields();
}

static LcdLightState_t *LCDLastLightingData(const lcdQueueElem_t *data)
{
    static LcdLightState_t LcdLightStatuses[5] = {0};

    if(data != NULL && data->operation== LCD_OP_LIGHT) {
        LcdLightStatuses[data->light.type] = data->light.state;
    }

    return LcdLightStatuses;
}

static LcdConnectionState_t LCDLastConnectionData(const lcdQueueElem_t *data)
{
    static LcdConnectionState_t LcdConnectionStatus = LCD_CONN_OFFLINE;

    if(data != NULL && data->operation == LCD_OP_CONN) {
        LcdConnectionStatus = data->conn.state;
    }

    return LcdConnectionStatus;
}

void AppTaskLCDControl(void *p_arg)
{
    OS_ERR os_err;
    lcdQueueElem_t *lcdQueueElemPtr = NULL;

    (void)p_arg;
    AppTaskLCDInit();

    OSTmrCreate(&lcdRefreshTimeout, "LCD refresh timeout", 3, 0, OS_OPT_TMR_ONE_SHOT, LCDRefreshCallback, 0, &os_err);
    OSTmrCreate(&lcdScrSaverTimeout, "LCD scrsaver timeout", 100, 0, OS_OPT_TMR_ONE_SHOT, LCDScrSaverCallback, 0, &os_err);
    OSTmrCreate(&lcdConnLockTimeout, "LCD connlock timeout", 5, 3, OS_OPT_TMR_PERIODIC, LCDConnLockCallback, 0, &os_err);

    LCDRefreshScreen();
    OSTmrStart((OS_TMR *)&lcdScrSaverTimeout, &os_err);
    while(1) {
        lcdQueueElemPtr = receive_from_lcd_queue();
        if(lcdQueueElemPtr != NULL) {
            if(scrSaverStarted == false) {
                if(lcdQueueElemPtr->operation == LCD_OP_CONN) {
                    if(connLockStarted == false) {
                        LCDDisplayData(lcdQueueElemPtr, 0);
                    }
                    LCDLastConnectionData(lcdQueueElemPtr);
                }
                else if(LCDLastConnectionData(NULL) == LCD_CONN_ONLINE) {
                    // heartbeat messages must not interfere with timers reloading
                    // otherwise SS will not be triggered
                    OSTmrStart((OS_TMR *)&lcdRefreshTimeout, &os_err);
                    OSTmrStart((OS_TMR *)&lcdScrSaverTimeout, &os_err);

                    if(connLockStarted == false) {
                        LCDDisplayData(lcdQueueElemPtr, 0);
                    }
                    LCDLastLightingData(lcdQueueElemPtr);
                    LCDLastConnectionData(lcdQueueElemPtr);
                }
                else {
                    OSTmrStart((OS_TMR *)&lcdConnLockTimeout, &os_err);
                    OSTmrStart((OS_TMR *)&lcdScrSaverTimeout, &os_err);
                }

            }
            else if(scrSaverStarted == true && lcdQueueElemPtr->operation != LCD_OP_CONN) {
                scrSaverStarted = false;

                OSTmrStart((OS_TMR *)&lcdRefreshTimeout, &os_err);
                OSTmrStart((OS_TMR *)&lcdScrSaverTimeout, &os_err);

                LCDRefreshScreen();

                if(LCDLastConnectionData(NULL) == LCD_CONN_ONLINE) {
                    LCDDisplayData(lcdQueueElemPtr, 0);
                    LCDLastLightingData(lcdQueueElemPtr);
                    LCDLastConnectionData(lcdQueueElemPtr);
                }
                else {
                    OSTmrStart((OS_TMR *)&lcdConnLockTimeout, &os_err);
                }
            }
            else {
                // heartbeat messages on SS enabled are being neglected!
            }
        }
        else {
            if(endActionRefresh == true) {
                endActionRefresh = false;
                LCDClearDataFields();
            }
            if(startScrSaver == true) {
                startScrSaver = false;
                scrSaverStarted = true;
                
                LCDDrawImage(SampleImage);
                // ignore all mesages on SS startup
                OSQFlush(&lcdQueue, &os_err);
            }
            if(startConnLock == true) {
                static uint8_t connLockCounter = 0;
                
                endActionRefresh = false;
                startConnLock = false;
                connLockStarted = true;
                
                connLockCounter++;
                if(connLockCounter == CONNECTION_LOCK_CNT_MAX) {
                    connLockCounter = 0;
                    connLockStarted = false;
                    OSTmrStop((OS_TMR *)&lcdConnLockTimeout, OS_OPT_TMR_NONE, 0, &os_err);
                }
                else {
                    static LcdConnectionState_t state = LCD_CONN_OFFLINE; 
                    lcdQueueElem_t data = {0};
                
                    if(state == LCD_CONN_OFFLINE) {
                        state = LCD_CONN_FREE;
                    }
                    else if(state == LCD_CONN_FREE) {
                        state = LCD_CONN_OFFLINE;
                        
                    }
                    else {
                        // no action defined
                    }
                    
                    data.operation = LCD_OP_CONN;
                    data.conn.state = state;
                    LCDDisplayData(&data, 0);
                }
            }
        }

        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_PERIODIC, &os_err);
    }
}


static void AppTaskLCDInit(void)
{
    OS_ERR err;
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = DATA | CLK | CS | RES;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB,&GPIO_InitStruct);

    // Hardware reset
    SetBit(CS);
    SetBit(CLK);
    SetBit(DATA);
    
    ClrBit(RES);
    OSTimeDlyHMSM(0, 0, 0, 5, OS_OPT_TIME_DLY, &err);
    SetBit(RES);
    OSTimeDlyHMSM(0, 0, 0, 5, OS_OPT_TIME_DLY, &err);

    // Sleep out
    WriteSpiCommand(SLEEPOUT);

    // Inversion off (command 0x20)
    WriteSpiCommand(INVOFF); 

    // Color Interface Pixel Format (command 0x3A)
    WriteSpiCommand(COLMOD);
    WriteSpiData(0x03); // 0x03 = 12 bits-per-pixel

    // Memory access controler (command 0x36)
    WriteSpiCommand(MADCTL);
    WriteSpiData(0x50);
    OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);

    // Display On (command 0x29)
    WriteSpiCommand(DISPON);
    
    SetContrast(60);
    //LCDClearScreen(WHITE);
}

static void SetBit(int bits)
{
    GPIO_SetBits(GPIOB, bits);
}

static void ClrBit(int bits)
{
    GPIO_ResetBits(GPIOB, bits);
}


static void WriteSpi(unsigned char data)
{
    unsigned char i = 0;
    SetBit(CLK);
    for (i = 0; i < 8; i++) 
    {
        ClrBit(CLK);
        if ((data&0x80))
            SetBit(DATA);
        else 
            ClrBit(DATA);
        SetBit(CLK);
        data <<= 1;
    }
    SetBit(CS);
}

static void WriteSpiCommand(unsigned char cmd)
{
    ClrBit(CS);
    ClrBit(CLK);
    ClrBit(DATA); // For commands the first bit is 0
    WriteSpi(cmd);
}


static void WriteSpiData(unsigned char data)
{
    ClrBit(CS);
    ClrBit(CLK);
    SetBit(DATA); // For data the first bit is 1
    WriteSpi(data);
}

static void SetContrast(unsigned char contr)
{
    WriteSpiCommand(SETCON);
    WriteSpiData(contr);
}

static void LCDClearScreen(LCD_color color) 
{
    int i = 0;
    // Set row address range
    WriteSpiCommand(PASET);
    WriteSpiData(0);
    WriteSpiData(131);

    // Set column address range
    WriteSpiCommand(CASET);
    WriteSpiData(0);
    WriteSpiData(131);

    // Write memory
    WriteSpiCommand(RAMWR);
    for(i = 0; i < (132*132)/2; i++) 
    {
          WriteSpiData((color  >> 4) & 0xFF); 
          WriteSpiData(((color  & 0xF) << 4) | ((color  >> 8) & 0xF)); 
          WriteSpiData(color  & 0xFF);  
    }
}

// Set x, y pixel to color
static void LCDSetPixel(int x, int y, unsigned int color)
{
    // Set columns address range
    WriteSpiCommand(CASET); 
    WriteSpiData(x);
    WriteSpiData(x);

    // Set rows address range
    WriteSpiCommand(PASET);
    WriteSpiData(y);
    WriteSpiData(y);

    // Write memory
    WriteSpiCommand(RAMWR);
    WriteSpiData((color  >> 4) & 0xFF); 
    WriteSpiData(((color  & 0xF) << 4) | ((color  >> 8) & 0xF)); 
    WriteSpiData(color  & 0xFF);
}

static void LCDLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, int color)
{
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;

    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }

    dy <<= 1;        // dy is now 2*dy 
    dx <<= 1;        // dx is now 2*dx 

    LCDSetPixel(x0, y0, color);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);  // same as 2*dy - dx 
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;    // same as fraction -= 2*dx 
            } 
            x0 += stepx;

            fraction += dy;    // same as fraction -= 2*dy 
            LCDSetPixel(x0, y0, color);
        }
    }
    else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            } 
            y0 += stepy;

            fraction += dx;
            LCDSetPixel(x0, y0, color);
        }
    }
}

static void LCDBox(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char fill, LCD_color color)
{ 
    unsigned char xmin = 0, xmax = 0, ymin = 0, ymax = 0; 
    int i = 0;

    xmin = (x0 <= x1) ? x0 : x1;  
    xmax = (x0 > x1) ? x0 : x1; 
    ymin = (y0 <= y1) ? y0 : y1; 
    ymax = (y0 > y1) ? y0 : y1;
    
    WriteSpiCommand(CASET);
    WriteSpiData(xmin);
    WriteSpiData(xmax);
    
    WriteSpiCommand(PASET); 
    WriteSpiData(ymin);
    WriteSpiData(ymax);
    
    WriteSpiCommand(RAMWR);
    for (i = 0; i < ((((xmax - xmin + 1) * (ymax - ymin + 1))/2 )+1 ); i++) { 
        WriteSpiData((color  >> 4) & 0xFF); 
        WriteSpiData(((color  & 0xF) << 4) | ((color  >> 8) & 0xF)); 
        WriteSpiData(color  & 0xFF);
    }

    WriteSpiCommand(NOP);
}

static void LCDDrawImage(const unsigned int image[132][132])
{
    int i = 0;
    unsigned int color1 = 0;
    unsigned int color2 = 0;
    const unsigned int *imagePtr = &image[0][0];
  
    // Set columns address range
    WriteSpiCommand(CASET); 
    WriteSpiData(0);
    WriteSpiData(131);

    // Set rows address range
    WriteSpiCommand(PASET);
    WriteSpiData(0);
    WriteSpiData(131);

    WriteSpiCommand(RAMWR);
    for(i=0; i < (132*132)/2; i++) {    
        color1 = *imagePtr;
        imagePtr++;
        color2 = *imagePtr;
        imagePtr++; 
        
        WriteSpiData((color1  >> 4) & 0xFF); 
        WriteSpiData(((color1  & 0xF) << 4) | ((color2  >> 8) & 0xF)); 
        WriteSpiData(color2  & 0xFF);
    }
    WriteSpiCommand(NOP);
    
    // alghoritm adjusted to NOKIA LCD Utility image generator
    //for(col=0;col<132;col++) {
    //    for(row=0;row<132;row++) {
     //       LCDSetPixel(row, col, image[131-col][row]);
     //   }
    //}
}

void LCDPutStr(char *pString, int  x, int  y, LCD_font_size Size, LCD_color fColor, LCD_color bColor)
{ 
       
    // loop until null-terminator is seen 
    while (*pString != 0x00) { 
        LCDPutChar(*pString++, x, y, Size, fColor, bColor);   
 
        // advance the y position 
        if (Size == SMALL) {
            x = x + 6; 
        }
        else if (Size == MEDIUM) {
            x = x + 8; 
        }
        else if(Size == LARGE) {
            x = x + 8; 
        }
        else {
            x = x + 12;
        }
   
        // bail out if y exceeds 131 
        if (x > 131) {
            break;
        }
    }
} 

void LCDPutChar(char c, int x, int y, LCD_font_size size, LCD_color fColor, LCD_color bColor)
{
    int i = 0,j = 0,k = 0, x_max = 0, y_max = 0;
    unsigned int nCols = 0;
    unsigned int nRows = 0;
    unsigned int nBytes   = 0;
    unsigned int PixelRow = 0;
    unsigned int Mask  = 0;
    unsigned int Word0 = 0;
    unsigned int Word1 = 0;
    unsigned char *pFont = NULL;
    unsigned char *pChar = NULL;
    unsigned char *FontTable[] = { (unsigned char *)FONT6x8, (unsigned char *)FONT8x8,
                                   (unsigned char *)FONT8x16, (unsigned char *)FONT12x16 };

    // get pointer to the beginning of the selected font table
    pFont = (unsigned char *)FontTable[size];
    // get the nColumns, nRows and nBytes
    nRows  = *pFont;
    nCols  = *(pFont + 1);
    nBytes = *(pFont + 2);
    x_max  = x + nRows - 1;
    y_max  = y + nCols - 1;

    // get pointer to the last byte of the desired character
    pChar = pFont + (nBytes * (c - 0x1F)) + nBytes - 1;

    // Row address set (command 0x2B)
    WriteSpiCommand(CASET);
    WriteSpiData(x);
    WriteSpiData(x_max);
    // Column address set (command 0x2A)
    WriteSpiCommand(PASET);
    WriteSpiData(y);
    WriteSpiData(y_max);
    
    // WRITE MEMORY
    WriteSpiCommand(RAMWR);
    // loop on each row, working backwards from the bottom to the top
    for (i = nCols - 1; i >= 0; i--) {
        // copy pixel row from font table and then decrement row
        PixelRow = *pChar--;
        // for fonts of width more than 8px
        if(nRows > 8) {
            PixelRow+=(*pChar--) * 256;
        }

        Mask=1;                 // instead of: pow(2,nCols-1): 2^(nCols-1), (takes less resources)
        for (k=0;k<nRows-1;k++) {
            Mask*=2;
        }

        // loop on each pixel in the row (left to right)
        // Note: we do two pixels each loop
        for (j = 0; j < nRows; j +=2) {
            // if pixel bit set, use foreground color; else use the background color
            // now get the pixel color for two successive pixels
            if ((PixelRow & Mask) == 0) {
                Word0 = bColor;
            }
            else {
                Word0 = fColor;
            }
                
            Mask = Mask >> 1;
            if ((PixelRow & Mask) == 0) {
                Word1 = bColor;
            }
            else {
                Word1 = fColor;
            }
            Mask = Mask >> 1;
            
            WriteSpiData((Word0 >> 4) & 0xFF);
            WriteSpiData(((Word0 & 0xF) << 4) | ((Word1 >> 8) & 0xF));
            WriteSpiData(Word1 & 0xFF);
        }
    }
    WriteSpiCommand(NOP);
}

void send_to_lcd_queue(const lcdQueueElem_t *lcdQueueNewElemPtr)
{
    static OS_ERR os_err;
    static lcdQueueElem_t *lcdQueueDestElemPtr = NULL;

    if(lcdQueueNewElemPtr->operation != LCD_OP_NONE) {
        lcdQueueDestElemPtr = &lcdQueueStruct.lcdQueueElem[lcdQueueStruct.lcdQueueElemNb];
        if(++lcdQueueStruct.lcdQueueElemNb >= LCD_QUEUE_SIZE) {
            lcdQueueStruct.lcdQueueElemNb = 0;
        }

        *lcdQueueDestElemPtr = *lcdQueueNewElemPtr;
        OSQPost(&lcdQueue, lcdQueueDestElemPtr, sizeof(*lcdQueueDestElemPtr), OS_OPT_POST_FIFO + OS_OPT_POST_ALL, &os_err);
    }
}

lcdQueueElem_t *receive_from_lcd_queue(void)
{
    static OS_ERR os_err;
    static OS_MSG_SIZE msg_size;
    
    lcdQueueElem_t *lcdQueueElemPtr = OSQPend(&lcdQueue, 0, OS_OPT_PEND_NON_BLOCKING, &msg_size, NULL, &os_err);

    return lcdQueueElemPtr;
}


/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
