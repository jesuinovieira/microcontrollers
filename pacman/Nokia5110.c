#include "Nokia5110.h"
#include "Symbols.h"


uint8_t Screen[SCREENW * SCREENH / 8]; // Buffer stores the next image to be printed on the screen
const unsigned char Masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}; // Utilizado na função Nokia5110_ClrPxl


// ================== PRIVATE FUNCTIONS ==================


// The Data/Command pin must be valid when the eighth bit is
// sent. The SSI module has hardware input and output FIFOs
// that are 8 locations deep. Based on the observation that
// the LCD interface tends to send a few commands and then a
// lot of data, the FIFOs are not used when writing
// commands, and they are used when writing data.  This
// ensures that the Data/Command pin status matches the byte
// that is actually being transmitted.
// The write command operation waits until all data has been
// sent, configures the Data/Command pin for commands, sends
// the command, and then waits for the transmission to
// finish.
// The write data operation waits until there is room in the
// transmit FIFO, configures the Data/Command pin for data,
// and then adds the data to the transmit FIFO.
// This is a helper function that sends an 8-bit message to the LCD.
// Inputs: type: COMMAND or DATA and message 8-bit code to transmit
// Assumes: SSI0 and port A have already been initialized and enabled
void static lcdwrite(enum typeOfWrite type, uint8_t message)
{
    if(type == COMMAND)
    {
        // Wait until SSI0 not busy/transmit FIFO empty
        while((SSI0_SR_R&SSI_SR_BSY) == SSI_SR_BSY){};
        DC = DC_COMMAND;
        SSI0_DR_R = message; // Command out
        
        // Wait until SSI0 not busy/transmit FIFO empty
        while((SSI0_SR_R&SSI_SR_BSY) == SSI_SR_BSY){};
    }

    else
    {
        while((SSI0_SR_R&SSI_SR_TNF) == 0){}; // Wait until transmit FIFO not full
        DC = DC_DATA;
        SSI0_DR_R = message;                  // Data out
    }
}


void static lcddatawrite(uint8_t data)
{
    while((SSI0_SR_R & 0x00000002) == 0){}; // Wait until transmit FIFO not full

    DC = DC_DATA;
    SSI0_DR_R = data;                       // Data out
}


// =================== PUBLIC FUNCTIONS ===================


// If the system clock is faster than 50 MHz, the SSI baud clock will be 
// faster than the 4 MHz maximum of the Nokia 5110.
// Assumes: system clock rate of 50 MHz or less [.h] ???
// Assumes: system clock rate of 80 MHz         [.c] ???
void Nokia5110_Init(void)
{
    volatile uint32_t delay;

    SYSCTL_RCGC1_R |= SYSCTL_RCGC1_SSI0;  // Activate SSI0
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // Activate port A
    
    delay = SYSCTL_RCGC2_R;               // Allow time to finish activating
    
    GPIO_PORTA_DIR_R |= 0xC0;             // Make PA6,7 out
    GPIO_PORTA_AFSEL_R |= 0x2C;           // Enable alt funct on PA2,3,5
    GPIO_PORTA_AFSEL_R &= ~0xC0;          // Disable alt funct on PA6,7
    GPIO_PORTA_DEN_R |= 0xEC;             // Enable digital I/O on PA2,3,5,6,7
    
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFF0F00FF) + 0x00202200; // Configure PA2,3,5 as SSI
    
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0x00FFFFFF) + 0x00000000; // Configure PA6,7 as GPIO
    
    GPIO_PORTA_AMSEL_R &= ~0xEC;          // Disable analog functionality on PA2,3,5,6,7
    SSI0_CR1_R &= ~SSI_CR1_SSE;           // Disable SSI
    SSI0_CR1_R &= ~SSI_CR1_MS;            // Master mode
    
    SSI0_CC_R = (SSI0_CC_R&~SSI_CC_CS_M)+SSI_CC_CS_SYSPLL; // Configure for system clock / PLL baud clock source

    // Clock divider for 3.33 MHz SSIClk (80 MHz PLL / 24)
    // SysClk/(CPSDVSR * (1 + SCR))
    // 80 / (24 * (1 + 0)) = 3.33 MHz (slower than 4 MHz)
    SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 24; // Must be even number
    SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (3.33 Mbps data rate)
                    SSI_CR0_SPH |         // SPH = 0
                    SSI_CR0_SPO);         // SPO = 0
    
    SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_FRF_M) + SSI_CR0_FRF_MOTO; // FRF = Freescale format
    SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_DSS_M) + SSI_CR0_DSS_8;    // DSS = 8-bit data
    SSI0_CR1_R |= SSI_CR1_SSE;                                     // Enable SSI

    RESET = RESET_LOW;                    // Reset the LCD to a known state
    for(delay=0; delay<10; delay=delay+1);// Delay minimum 100 ns
    RESET = RESET_HIGH;                   // Negative logic

    lcdwrite(COMMAND, 0x21);              // Chip active; horizontal addressing mode (V = 0); use extended instruction set (H = 1)

    // Set LCD Vop (contrast), which may require some tweaking:
    lcdwrite(COMMAND, CONTRAST);          // Try 0xB1 (for 3.3V red SparkFun), 0xB8 (for 3.3V blue SparkFun), 0xBF if your display is too dark, or 0x80 to 0xFF if experimenting
    lcdwrite(COMMAND, 0x04);              // Set temp coefficient
    lcdwrite(COMMAND, 0x14);              // LCD bias mode 1:48: try 0x13 or 0x14

    lcdwrite(COMMAND, 0x20);              // We must send 0x20 before modifying the display control mode
    lcdwrite(COMMAND, 0x0C);              // Set display control to normal mode: 0x0D for inverse
}


// The character will be printed at the current cursor position, the cursor
// will automatically be updated, and it will wrap to the next row or back
// to the top if necessary. One blank column of pixels will be printed on 
// either side of the character for readability.
void Nokia5110_OutChar(char data)
{
    int i;
    lcddatawrite(0x00); // Blank vertical line padding

    for(i = 0; i < 5; i = i + 1)
        lcddatawrite(ASCII[data - 0x20][i]);

    lcddatawrite(0x00); // Blank vertical line padding
}


// The string will automatically wrap, so padding spaces may
// be needed to make the output look optimal.
void Nokia5110_OutString(char *ptr)
{
    while(*ptr)
    {
        Nokia5110_OutChar((unsigned char) * ptr);
        ptr = ptr + 1;
    }
}


// Output a 16-bit number in unsigned decimal format with a
// fixed size of five right-justified digits of output.
void Nokia5110_OutUDec(uint16_t n)
{
    if(n < 10)
    {
        Nokia5110_OutString("    ");
        Nokia5110_OutChar(n + '0'); // n is between 0 and 9 
    } 
    
    else if(n < 100)
    {
        Nokia5110_OutString("   ");
        Nokia5110_OutChar(n / 10 + '0'); // Tens digit
        Nokia5110_OutChar(n % 10 + '0'); // Ones digit
    }
    
    else if(n < 1000)
    {
        Nokia5110_OutString("  ");
        Nokia5110_OutChar(n / 100 + '0'); n = n % 100;  // Hundreds digit
        
        Nokia5110_OutChar(n / 10 + '0');                // Tens digit
        Nokia5110_OutChar(n % 10 + '0');                // Ones digit
    }
    
    else if(n < 10000)
    {
        Nokia5110_OutChar(' ');
        Nokia5110_OutChar(n / 1000 + '0'); n = n % 1000; // Thousands digit

        Nokia5110_OutChar(n / 100 + '0'); n = n % 100;   // Hundreds digit

        Nokia5110_OutChar(n / 10 + '0');                 // Tens digit
        Nokia5110_OutChar(n % 10 + '0');                 // Ones digit
    }
    
    else
    {
        Nokia5110_OutChar(n / 10000 + '0'); n = n % 10000;  // Ten-thousands digit
        
        Nokia5110_OutChar(n / 1000 + '0'); n = n % 1000;    // Thousands digit
        
        Nokia5110_OutChar(n / 100 + '0'); n = n % 100;      // Hundreds digit
        
        Nokia5110_OutChar(n / 10 + '0');                    // Tens digit
        Nokia5110_OutChar(n % 10 + '0');                    // Ones digit
    }
}


// This function set the cursor for ASCII characters
// X = 0 is the leftmost column.  
// Y = 0 is the top row.
void Nokia5110_SetCursor(uint8_t newX, uint8_t newY)
{
    if((newX > 11) || (newY > 5))        // Bad input
        return;

    // Multiply newX by 7 because each character is 7 columns wide
    lcdwrite(COMMAND, 0x80 | (newX * 7));   // Setting bit 7 updates X-position
    lcdwrite(COMMAND, 0x40 | newY);         // Setting bit 6 updates Y-position
}


// Clear the LCD by writing zeros to the entire screen and
// reset the cursor to (0,0) (top left corner of screen).
void Nokia5110_Clear(void)
{
    int i;

    for(i = 0; i < (MAX_X * MAX_Y / 8); i = i + 1)
        lcddatawrite(0x00);
    
    Nokia5110_SetCursor(0, 0);
}


// Fill the whole screen by drawing a 48x84 bitmap image.
// Inputs: ptr pointer to 504 byte bitmap
void Nokia5110_DrawFullImage(const uint8_t *ptr)
{
    int i;

    Nokia5110_SetCursor(0, 0);

    for(i = 0; i < (MAX_X * MAX_Y / 8); i = i + 1)
        lcddatawrite(ptr[i]);
}


// The image will appear on the screen after the next call to Nokia5110_DisplayBuffer();
// threshold: grayscale colors above this number make corresponding pixel 'on' 0 to 14
// 0 is fine for ships, explosions, projectiles, and bunkers
void Nokia5110_PrintBMP(uint8_t xpos, uint8_t ypos, const uint8_t *ptr, uint8_t threshold)
{
    // A formatação do BitMap deve ser tal que:
    // ptr[18] deve conter a largura
    // ptr[22] deve conter a altura

    int32_t width = ptr[18], height = ptr[22], i, j;
    uint16_t screenx, screeny;
    uint8_t mask;

    // Check for clipping
    if((height <= 0) ||                 // Bitmap is unexpectedly encoded in top-to-bottom pixel order
      ((width % 2) != 0) ||             // Must be even number of columns
      ((xpos + width) > SCREENW) ||     // Right side cut off
      (ypos < (height - 1)) ||          // Top cut off
      (ypos > SCREENH))                 // Bottom cut off
    {
        return;
    }
    
    if(threshold > 14)
        threshold = 14;                 // Only full 'on' turns pixel on


    // Bitmaps are encoded backwards, so start at the bottom left corner of the image
    
    screeny = ypos / 8;
    screenx = xpos + SCREENW * screeny;

    mask = ypos % 8;                // Row 0 to 7
    mask = 0x01 << mask;            // Now stores a mask 0x01 to 0x80
    j = ptr[10];                    // Byte 10 contains the offset where image data can be found
    
    for(i = 1; i <= (width * height / 2); i = i + 1)
    {
        // The left pixel is in the upper 4 bits
        if(((ptr[j] >> 4) & 0xF) > threshold)   Screen[screenx] |= mask;
        else                                    Screen[screenx] &= ~mask;

        screenx = screenx + 1;
        
        // The right pixel is in the lower 4 bits
        if((ptr[j] & 0xF) > threshold)  Screen[screenx] |= mask;
        else                            Screen[screenx] &= ~mask;

        screenx = screenx + 1;
        j = j + 1;

        if((i % (width / 2)) == 0)     // At the end of a row
        {
            if(mask > 0x01) mask = mask >> 1;
            
            else
            {
                mask = 0x80;
                screeny = screeny - 1;
            }
            
            screenx = xpos + SCREENW * screeny;
            
            // Bitmaps are 32-bit word aligned
            
            switch((width / 2) % 4) // Skip any padding
            {
                case 0: j = j + 0; break;
                case 1: j = j + 3; break;
                case 2: j = j + 2; break;
                case 3: j = j + 1; break;
            }
        }
    }
}


// There is a buffer in RAM that holds one screen. This routine clears this buffer
void Nokia5110_ClearBuffer(void)
{
    int i;
    for(i = 0; i < SCREENW * SCREENH / 8; i = i + 1)
        Screen[i] = 0;              // clear buffer
}


// Fill the whole screen by drawing a 48x84 screen image.
void Nokia5110_DisplayBuffer(void)
{
    Nokia5110_DrawFullImage(Screen);
}


// Clear the Image pixel at (i, j), turning it dark.
// i the column index (0 to 83 in this case), x-coordinate
// j the row index (0 to 47 in this case), y-coordinate
void Nokia5110_ClearPixel(uint32_t i, uint32_t j)
{
    Screen[84 * (j >> 3) + i] &= ~Masks[j & 0x07];
}


// Set the Image pixel at (i, j), turning it on.
// i the COLUMN index (0 to 83 in this case), x-coordinate
// j the LINE index (0 to 47 in this case), y-coordinate
void Nokia5110_DrawPixel(uint32_t i, uint32_t j)
{
    Screen[84 * (j >> 3) + i] |= Masks[j & 0x07];
}


// Configure the system to get its clock from the PLL
void PLL_Init(void)
{
    // 0) Configure the system to use RCC2 for advanced features
    //    Such as 400 MHz PLL and non-integer System Clock Divisor
    SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;

    // 1) Bypass PLL while initializing
    SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;
    
    // 2) Select the crystal value and oscillator source
    SYSCTL_RCC_R &= ~SYSCTL_RCC_XTAL_M;         // Clear XTAL field
    SYSCTL_RCC_R += SYSCTL_RCC_XTAL_16MHZ;      // Configure for 16 MHz crystal
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;    // Clear oscillator source field
    SYSCTL_RCC2_R += SYSCTL_RCC2_OSCSRC2_MO;    // Configure for main oscillator source
    
    // 3) Activate PLL by clearing PWRDN
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;
    
    // 4) Set the desired system divider and the system divider least significant bit
    SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400;            // Use 400 MHz PLL
    SYSCTL_RCC2_R = (SYSCTL_RCC2_R & ~0x1FC00000)   // Clear system clock divider field
                          + (SYSDIV2<<22);          // Configure for 80 MHz clock
    
    // 5) Wait for the PLL to lock by polling PLLLRIS
    while((SYSCTL_RIS_R & SYSCTL_RIS_PLLLRIS) == 0){};
    
    // 6) Enable use of PLL by clearing BYPASS
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;
}


// ===================== MODIFICATIONS =====================


// ???
void Nokia5110_OutDec(uint16_t n)
{
    if(n < 10)
    {
        //Nokia5110_OutString("    ");
        Nokia5110_OutChar(n + '0'); // n is between 0 and 9
    }

    else if(n < 100)
    {
        //Nokia5110_OutString("   ");
        Nokia5110_OutChar(n/10+'0'); // tens digit
        Nokia5110_OutChar(n%10+'0'); // ones digit 
    }

    else if(n < 1000)
    {
        //Nokia5110_OutString("  ");
        Nokia5110_OutChar(n / 100 + '0'); n = n % 100; // Hundreds digit

        Nokia5110_OutChar(n / 10 + '0'); // tens digit
        Nokia5110_OutChar(n % 10 + '0'); // ones digit
    }

    else if(n < 10000)
    {
        //Nokia5110_OutChar(' ');
        Nokia5110_OutChar(n / 1000 + '0'); n = n % 1000; // Thousands digit

        Nokia5110_OutChar(n / 100 + '0'); n = n % 100;// Hundreds digit

        Nokia5110_OutChar(n / 10 + '0'); // Tens digit
        Nokia5110_OutChar(n % 10 + '0'); // Ones digit
    }

    else
    {
        Nokia5110_OutChar(n / 10000 + '0'); n = n % 10000;  // Ten-thousands digit

        Nokia5110_OutChar(n / 1000 + '0'); n = n % 1000;    // Thousands digit

        Nokia5110_OutChar(n / 100 + '0'); n = n % 100;      // Hundreds digit

        Nokia5110_OutChar(n / 10 + '0');                    // Tens digit
        Nokia5110_OutChar(n % 10 + '0');                    // Ones digit
    }
}


// ???
void Nokia5110_OutCharInv(char data)
{
    int i;
    lcddatawrite(0x00);        // Blank vertical line padding

    for(i = 4; i > -1; i--)
    {
        char value = 0, j, aux = ASCII[data - 0x20][i];
        for(j = 0; j < 8; j++) value |= (((aux & (0x80 >> j)) != 0) << j);
        lcddatawrite(value);
    }

    lcddatawrite(0x00);        // Blank vertical line padding
}


// ???
void Nokia5110_OutStringInv(char *ptr)
{
    int i = 0;

    while(ptr[i] != '\0') i++;

    for(i--; i > -1; i--) Nokia5110_OutCharInv(ptr[i]);
}


// ???
void Nokia5110_OutSpecial(uint16_t code)
{
    int i;
    lcddatawrite(0x00);        // blank vertical line padding
    for(i = 0; i < 5; i = i + 1) lcddatawrite(SPECIAL_SIMBOLS[code][i]);

    lcddatawrite(0x00);        // blank vertical line padding
}


// ???
void Nokia5110_DrawChar(char data)
{
    int i;
    for(i = 0; i < 5; i++) lcddatawrite(ASCII[data - 0x20][i]);
}


//
void Nokia5110_DrawHLine(uint8_t x, uint8_t xMax, uint8_t y)
{
    if(xMax < x || xMax > 83 || y > 47) return;

    uint8_t i;
    for(i = x; i < xMax; i++) Nokia5110_DrawPixel(i, y);
}


//
void Nokia5110_DrawVLine(uint8_t y, uint8_t yMax, uint8_t x)
{
    if(yMax < y || yMax > 48 || x > 83) return;

    uint8_t j;
    for(j = y; j < yMax; j++) Nokia5110_DrawPixel(x, j);
}


// Set pixel cursor
// X = 0 is the leftmost column.  
// Y = 0 is the top row.
void Nokia5110_SetPixel(uint8_t newX, uint8_t newY)
{
    if(newX > 83 || newY > 47) // Bad input
        return;

    lcdwrite(COMMAND, 0x80 | newX);   // Setting bit 7 updates X-position
    lcdwrite(COMMAND, newY);   // Setting bit 6 updates Y-position
}


//
void Nokia5110_ClearBitmap(uint8_t xpos, uint8_t ypos, const uint8_t *ptr)
{
    if(xpos > 83 || ypos > 47 || !ptr)
        return;

    uint8_t i, j;
    uint8_t width = ptr[18], height = ptr[22];

    for(i = xpos; i < xpos + width; i++)
        for(j = ypos; j > ypos - height; j--)
            Nokia5110_ClearPixel(i, j);
}


//
void Nokia5110_ClearSquare(uint8_t xpos, uint8_t ypos, uint8_t size)
{
    if(xpos + size > 83 || xpos > 83 || ypos + size > 47 || ypos > 47)
        return;

    uint8_t i, j;

    for(i = xpos; i < xpos + size; i++)
        for(j = ypos; j > ypos - size; j--)
            Nokia5110_ClearPixel(i, j);
}


// NÃO TA FUNCIONANDO | NÃO TA FUNCIONANDO | NÃO TA FUNCIONANDO
// Return true if the pixel is off
// Return false if the pixel is on
// NÃO TA FUNCIONANDO | NÃO TA FUNCIONANDO | NÃO TA FUNCIONANDO
bool Nokia5110_ReadPixel(uint8_t i, uint8_t j)
{
    return Screen[84 * (j >> 3) + i] == 0 ? true : false;
}


// =======================================================================
// UNUSED

// DRAWPACMAN
void Nokia5110_DrawPacMan(uint16_t code)
{
    int i;
    for(i = 0; i < 7; i++) lcddatawrite(PAC_MAN[code][i]);
}


// ???
void Nokia5110_DrawSpecial(uint16_t code)
{
    int i;
    for(i = 0; i < 6; i++) lcddatawrite(SPECIAL_SIMBOLS[code][i]);
}


// ???
void Nokia5110_DrawMix(uint8_t code1, uint8_t code2, uint8_t code3, uint8_t code4)
{
    int i;
    for(i = 0; i < 2; i++) lcddatawrite((MIX_SIMBOLS[code1][i] << 6) | (MIX_SIMBOLS[code2][i] << 4) |
                                        (MIX_SIMBOLS[code3][i] << 2) | (MIX_SIMBOLS[code4][i]));
}


// Preenchimento ???
void Nokia5110_Padding()
{
    lcddatawrite(0x00);
}
