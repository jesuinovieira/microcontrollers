/* 
    This example accompanies the book
    "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
    ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

    Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
    THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
    OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
    VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
    OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
    For more information about my classes, my research, and my books, see
    http://users.ece.utexas.edu/~valvano/

    Modified by:    Jesuino Vieira Filho
    GitHub:         https://github.com/jesuinovieira
 
    ======================== PINS ========================
    Blue Nokia 5110
    -------------------------------------------
    Signal        (Nokia 5110) LaunchPad pin
    -------------------------------------------
    Reset         (RST, pin 1) connected to PA7
    SSI0Fss       (CE,  pin 2) connected to PA3
    Data/Command  (DC,  pin 3) connected to PA6
    SSI0Tx        (Din, pin 4) connected to PA5
    SSI0Clk       (Clk, pin 5) connected to PA2
    3.3V          (Vcc, pin 6) power
    back light    (BL,  pin 7) not connected
    Ground        (Gnd, pin 8) ground
*/

#ifndef NOKIA5110_H
#define NOKIA5110_H

#include <stdint.h>
#include <stdbool.h>
// #include "driverlib/rom.h"
// #include "driverlib/lcd.h"
// #include "driverlib/ssi.h"
// #include "driverlib/gpio.h"
// #include "driverlib/pin_map.h"   // Mapping of peripherals to pins for all parts. "GPIOPinTypeUART"
// #include "driverlib/sysctl.h"
// #include "driverlib/interrupt.h"
// #include "inc/hw_gpio.h"
// #include "inc/hw_memmap.h"
// #include "inc/hw_types.h"
// #include "inc/hw_ints.h"
// #include "inc/hw_ssi.h"



// ========================================================
enum typeOfWrite
{
    COMMAND,                              // The transmission is an LCD command
    DATA                                  // The transmission is data
};

// The DC pin tells the LCD if we are sending a command or data
#define LCD_COMMAND 0
#define LCD_DATA  1

// You may find a different size screen, but this one is 84 by 48 pixels
#define LCD_X     84
#define LCD_Y     48



// ===================== WINDOWS SIZE =====================
#define MAX_X                   84
#define MAX_Y                   48
#define CONTRAST                0xB7
#define SCREENW     84
#define SCREENH     48



// ======================== DEFINES ========================
#define DC                      (*((volatile uint32_t *)0x40004100))
#define DC_COMMAND              0
#define DC_DATA                 0x40
#define RESET                   (*((volatile uint32_t *)0x40004200))
#define RESET_LOW               0
#define RESET_HIGH              0x80
#define GPIO_PORTA_DIR_R        (*((volatile uint32_t *)0x40004400))
#define GPIO_PORTA_AFSEL_R      (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTA_AMSEL_R      (*((volatile uint32_t *)0x40004528))
#define GPIO_PORTA_PCTL_R       (*((volatile uint32_t *)0x4000452C))
#define SSI0_CR0_R              (*((volatile uint32_t *)0x40008000))
#define SSI0_CR1_R              (*((volatile uint32_t *)0x40008004))
#define SSI0_DR_R               (*((volatile uint32_t *)0x40008008))
#define SSI0_SR_R               (*((volatile uint32_t *)0x4000800C))
#define SSI0_CPSR_R             (*((volatile uint32_t *)0x40008010))
#define SSI0_CC_R               (*((volatile uint32_t *)0x40008FC8))
#define SSI_CR0_SCR_M           0x0000FF00  // SSI Serial Clock Rate
#define SSI_CR0_SPH             0x00000080  // SSI Serial Clock Phase
#define SSI_CR0_SPO             0x00000040  // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M           0x00000030  // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO        0x00000000  // Freescale SPI Frame Format
#define SSI_CR0_DSS_M           0x0000000F  // SSI Data Size Select
#define SSI_CR0_DSS_8           0x00000007  // 8-bit data
#define SSI_CR1_MS              0x00000004  // SSI Master/Slave Select
#define SSI_CR1_SSE             0x00000002  // SSI Synchronous Serial Port Enable
#define SSI_SR_BSY              0x00000010  // SSI Busy Bit
#define SSI_SR_TNF              0x00000002  // SSI Transmit FIFO Not Full
#define SSI_CPSR_CPSDVSR_M      0x000000FF  // SSI Clock Prescale Divisor
#define SSI_CC_CS_M             0x0000000F  // SSI Baud Clock Source
#define SSI_CC_CS_SYSPLL        0x00000000  // Either the system clock (if the PLL bypass is in effect) or the PLL output (default)
#define SYSCTL_RCGC1_R          (*((volatile uint32_t *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile uint32_t *)0x400FE108))
#define SYSCTL_RCGC1_SSI0       0x00000010  // SSI0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // Port A Clock Gating Control



// ======================== DEFINES PLL ======================== (Obs.: dif)
#define SYSCTL_RIS_R            (*((volatile uint32_t *)0x400FE050))
#define SYSCTL_RIS_PLLLRIS      0x00000040  // PLL Lock Raw Interrupt Status
#define SYSCTL_RCC_R            (*((volatile uint32_t *)0x400FE060))
#define SYSCTL_RCC_XTAL_M       0x000007C0  // Crystal Value
#define SYSCTL_RCC_XTAL_6MHZ    0x000002C0  // 6 MHz Crystal
#define SYSCTL_RCC_XTAL_8MHZ    0x00000380  // 8 MHz Crystal
#define SYSCTL_RCC_XTAL_16MHZ   0x00000540  // 16 MHz Crystal
#define SYSCTL_RCC2_R           (*((volatile uint32_t *)0x400FE070))
#define SYSCTL_RCC2_USERCC2     0x80000000  // Use RCC2
#define SYSCTL_RCC2_DIV400      0x40000000  // Divide PLL as 400 MHz vs. 200 MHz
#define SYSCTL_RCC2_SYSDIV2_M   0x1F800000  // System Clock Divisor 2
#define SYSCTL_RCC2_SYSDIV2LSB  0x00400000  // Additional LSB for SYSDIV2
#define SYSCTL_RCC2_PWRDN2      0x00002000  // Power-Down PLL 2
#define SYSCTL_RCC2_BYPASS2     0x00000800  // PLL Bypass 2
#define SYSCTL_RCC2_OSCSRC2_M   0x00000070  // Oscillator Source 2
#define SYSCTL_RCC2_OSCSRC2_MO  0x00000000  // MOSC

#define SYSDIV2 7



// =================== DEFINITIONS USED IN THE EXAMPLE ===================
#define PIN_SCE   7 // Pin 3 on LCD
#define PIN_RESET 6 // Pin 4 on LCD
#define PIN_DC    5 // Pin 5 on LCD
#define PIN_SDIN  4 // Pin 6 on LCD
#define PIN_SCLK  3 // Pin 7 on LCD




// ======================== FUNCTIONS PROTOTYPES ========================


void Nokia5110_Init             (void);
void Nokia5110_OutChar          (char data);
void Nokia5110_OutString        (char *ptr);
void Nokia5110_OutUDec          (uint16_t n);
void Nokia5110_OutDec           (uint16_t n);
void Nokia5110_SetCursor        (uint8_t newX, uint8_t newY);
void Nokia5110_Clear            (void);
void Nokia5110_DrawFullImage    (const uint8_t *ptr);
void Nokia5110_PrintBMP         (uint8_t xpos, uint8_t ypos, const uint8_t *ptr, uint8_t threshold);
void Nokia5110_ClearBuffer      (void);
void Nokia5110_DisplayBuffer    (void);
void Nokia5110_ClearPixel       (uint32_t, uint32_t);
void Nokia5110_DrawPixel        (uint32_t, uint32_t);
void Nokia5110_DrawMainCar      (const uint8_t *img, int x, int y);
void Nokia5110_SetScreenPxl     (uint8_t *ptr, uint8_t data, int pos);
void PLL_Init                   (void);

// void Nokia5110_DrawBlack         (void);
// void Nokia5110_DrawFullImageInv  (const uint8_t *img);

void Nokia5110_OutCharInv       (char data);
void Nokia5110_OutStringInv     (char *ptr);
void Nokia5110_OutSpecial       (uint16_t code);
void Nokia5110_DrawChar         (char data);
void Nokia5110_DrawSpecial      (uint16_t code);
void Nokia5110_DrawMix          (uint8_t code1, uint8_t code2, uint8_t code3, uint8_t code4);
void Nokia5110_Padding          (void);


void Nokia5110_DrawVLine(uint8_t, uint8_t, uint8_t);
void Nokia5110_DrawHLine(uint8_t, uint8_t, uint8_t);


void Nokia5110_DrawPacMan       (uint16_t);
void Nokia5110_SetPixel         (uint8_t, uint8_t);
void Nokia5110_ClearBitmap      (uint8_t, uint8_t, const uint8_t *);
void Nokia5110_ClearSquare      (uint8_t, uint8_t, uint8_t);

bool Nokia5110_ReadPixel        (uint8_t, uint8_t);

/*
void Nokia5110_InvertPixel      (uint8_t, uint8_t);
*/


#endif
