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
#include <includes.h>


static void AppTaskLCDInit(void);
static void SetBit(int bits);
static void ClrBit(int bits);
static void WriteSpi(unsigned char data);
static void WriteSpiCommand(unsigned char cmd);
static void WriteSpiData(unsigned char data);
static void SetContrast(unsigned char contr);
static void LCDClearScreen(LCD_color color);
static void LCDSetPixel(int x, int y, unsigned int color);
static void LCDPutChar(char c, int  x, int  y, LCD_font_size size, LCD_color fColor, LCD_color bColor);
static void LCDPutStr(char *pString, int  x, int  y, LCD_font_size Size, LCD_color fColor, LCD_color bColor);

void AppTaskLCDControl(void *p_arg)
{
    OS_ERR os_err;
    (void)p_arg;

    AppTaskLCDInit();
    while(1) {

        //SPI_I2S_SendData(SPI1, 123);
        //while( SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET ||
        //       SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET    );
        
        OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_PERIODIC, &os_err);
    }
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
    for(i = 0; i < (131 * 131); i++) 
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

void LCD_Box(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char fill, int color)
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
    int row = 0;
    int col = 0;
  
    for(col=0;col<132;col++) {
        for(row=0;row<132;row++) {
            LCDSetPixel(row, col, image[131-col][131-row]);
        }
    }
}

static void AppTaskLCDInit(void)
{
    int i = 0;
    GPIO_InitTypeDef GPIO_InitStruct;
//    SPI_InitTypeDef SPI_InitStruct;

//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

//    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
//    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
//    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//    GPIO_Init(GPIOB, &GPIO_InitStruct);

//    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
//    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
//    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);

//    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // set to full duplex
//    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;     // transmit in master mode, NSS pin has
//    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b; // one packet of data is 8 bits wide
//    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;        // clock is low when idle
//    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;      // data sampled at first edge
//    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft; // set the NSS HARD
//    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; // SPI frequency is
//    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;// data is transmitted MSB first
//    //SPI_InitStruct.SPI_CRCPolynomial = 7;
//    SPI_Init(SPI1, &SPI_InitStruct);

//    //SPI_SSOutputCmd(SPI1,ENABLE); //Set SSOE bit in SPI_CR1 register
//    SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, ENABLE);
//    SPI_Cmd(SPI1, ENABLE); // enable SPI1

    GPIO_InitStruct.GPIO_Pin= GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_UP;
    GPIO_Init(GPIOB,&GPIO_InitStruct);

    // Hardware reset
    SetBit(CS);
    SetBit(CLK);
    SetBit(DATA);
    ClrBit(RES);
    for(i = 0; i < 800000; i++);
    SetBit(RES);
    for(i = 0; i < 800000; i++);

    // Sleep out
    WriteSpiCommand(SLEEPOUT);

    // Inversion on (command 0x20)
    WriteSpiCommand(INVOFF); 

    // Color Interface Pixel Format (command 0x3A)
    WriteSpiCommand(COLMOD);
    WriteSpiData(0x03); // 0x03 = 12 bits-per-pixel

    // Memory access controler (command 0x36)
    WriteSpiCommand(MADCTL);
    WriteSpiData(0x50);

    for(i = 0; i < 20000; i++);

    // Display On (command 0x29)
    WriteSpiCommand(DISPON);
    
    SetContrast(62);
    LCDClearScreen(WHITE);
    
    for(i = 0; i < 800000; i++);
    LCDDrawImage(SampleImage);
    LCD_Box(0, 0, 100, 40, 1, RED);
    LCDPutStr("Hello World!", 20, 20, SMALL, BLACK, WHITE);
    LCDPutStr("Hello World!", 40, 20, MEDIUM, BLACK, WHITE);
    LCDPutStr("Hello World!", 60,5, LARGE, BLACK, WHITE);
    LCDPutStr("Hello World!", 80,5, HUGE, BLACK, WHITE);

}

void LCDPutStr(char *pString, int  x, int  y, LCD_font_size Size, LCD_color fColor, LCD_color bColor)
{ 
       
    // loop until null-terminator is seen 
    while (*pString != 0x00) { 
        LCDPutChar(*pString++, x, y, Size, fColor, bColor);   
 
        // advance the y position 
        if (Size == SMALL) {
            y = y + 6; 
        }
        else if (Size == MEDIUM) {
            y = y + 7; 
        }
        else if(Size == LARGE) {
            y = y + 8; 
        }
        else {
            y = y + 12;
        }
   
        // bail out if y exceeds 131 
        if (y > 131) {
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
    nCols  = *pFont;
    nRows  = *(pFont + 1);
    nBytes = *(pFont + 2);
    x_max  = x + nRows - 1;
    y_max  = y + nCols - 1;

    // get pointer to the last byte of the desired character
    pChar = pFont + (nBytes * (c - 0x1F)) + nBytes - 1;

    // Row address set (command 0x2B)
    WriteSpiCommand(PASET);
    WriteSpiData(x);
    WriteSpiData(x_max);
    // Column address set (command 0x2A)
    WriteSpiCommand(CASET);
    WriteSpiData(y);
    WriteSpiData(y_max);
    // WRITE MEMORY
    WriteSpiCommand(RAMWR);
    
    // loop on each row, working backwards from the bottom to the top
    for (i = nRows - 1; i >= 0; i--) {
        // copy pixel row from font table and then decrement row
        PixelRow = *pChar--;
        // for fonts of width more than 8px
        if(nCols > 8) {
            PixelRow+=(*pChar--) * 256;
        }

        Mask=1;                 // instead of: pow(2,nCols-1): 2^(nCols-1), (takes less resources)
        for (k=0;k<nCols-1;k++) {
            Mask*=2;
        }

        // loop on each pixel in the row (left to right)
        // Note: we do two pixels each loop
        for (j = 0; j < nCols; j +=2) {
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


/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
