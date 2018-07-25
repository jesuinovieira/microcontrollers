#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Nokia5110.h"
#include "Bitmaps.h"
#include "Buttons.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

// ====================================================================================


#define PACMAN_RIGHT             0
#define PACMAN_LEFT              1
#define PACMAN_UP                2
#define PACMAN_DOWN              3
#define SW_OK                    4

struct Character
{
	uint8_t xPos;
	uint8_t yPos;
	
	const unsigned char *BMP[4];
	uint8_t direction;
	uint8_t life;
};


// ====================================================================================


typedef struct Character Character;

void Initialize         (void);
void InitializeGrid     (void);
void Move               (void);
void DrawCharacter      (void);
void EraseCharacter     (void);
void attTime            (void);
void Introduction       (void);
void Pause              (void);
void EndGame            (void);
void CheckEndGame       (void);
void EatFruit           (uint8_t, uint8_t);
bool CheckMovement      (uint8_t, uint8_t);
uint8_t ConvertSwitch   (uint8_t);

// ====================================================================================


extern uint8_t Map      [41][84];
extern uint8_t Fruits   [41][84];
const unsigned char *Numbers[] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE};

bool GamePaused = false;
uint32_t Score = 0;
uint32_t BestScore = 0;
uint32_t Time = 0;
Character PacMan;


// ====================================================================================
// Main

int main(void)
{
	// No SpaceInvaders --- 80 MHz
	// Acho que é muito alto.. ideal seria uns 50MHz?! Talvez eu tenha lido isso em algum lugar (TESTAR)
	SysCtlClockSet(SYSCTL_SYSDIV_20 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

	Nokia5110_Init();

	Introduction();
    Initialize();
    SysCtlDelay(SysCtlClockGet() / 2);

    while(true)
    {
        Move();
        SysCtlDelay(SysCtlClockGet() / 100);
    }
}


// ====================================================================================
// Functions

void Timer0AIntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    if(GamePaused != true) attTime();
}


void attTime(void)
{
    Time = Time + 1;
    Nokia5110_PrintBMP(20, 4, Numbers[Time / 1000], 0);
    Nokia5110_PrintBMP(24, 4, Numbers[(Time % 1000) / 100], 0);
    Nokia5110_PrintBMP(28, 4, Numbers[((Time % 1000) % 100) / 10], 0);
    Nokia5110_PrintBMP(32, 4, Numbers[((Time % 1000) % 100) % 10], 0);
}


void Introduction(void)
{
    // Configure buttons
    ConfigureButtons();

    Nokia5110_Clear();
    Nokia5110_ClearBuffer();

    Nokia5110_PrintBMP(0, 47, BMPIntroduction, 0);
    Nokia5110_DisplayBuffer();
    SysCtlDelay(2 * SysCtlClockGet());

    // ________________________________________

    Pause();

    // ________________________________________

    Nokia5110_PrintBMP(0, 47, BMPPressEnterToPlay, 0);
    Nokia5110_DisplayBuffer();

    SysCtlDelay(SysCtlClockGet() / 5);
    while(ConvertSwitch(GetButton()) == BUTTON_NOT_PRESSED){}
    SysCtlDelay(SysCtlClockGet() / 5);

    Nokia5110_Clear();
    Nokia5110_ClearBuffer();
}


void Pause(void)
{
    Nokia5110_PrintBMP(0, 47, BMPHelp, 0);
    Nokia5110_DisplayBuffer();

    if(Time > 1) GamePaused = 1;

    SysCtlDelay(SysCtlClockGet() / 5);
    while(ConvertSwitch(GetButton()) != SW_OK){}

    GamePaused = 0;

    Nokia5110_Clear();
    Nokia5110_ClearBuffer();
}


void Initialize(void)
{
	PacMan.xPos = 2;
	PacMan.yPos = 14;
	PacMan.life = 1;
	PacMan.direction = PACMAN_RIGHT;

    PacMan.BMP[PACMAN_RIGHT]    = BMPPacMan_R;
    PacMan.BMP[PACMAN_LEFT]     = BMPPacMan_L;
    PacMan.BMP[PACMAN_UP]       = BMPPacMan_U;
    PacMan.BMP[PACMAN_DOWN]     = BMPPacMan_D;

    InitializeGrid();

    EatFruit(PacMan.xPos, PacMan.yPos);
    Nokia5110_PrintBMP(PacMan.xPos, PacMan.yPos, PacMan.BMP[ PacMan.direction ], 0);

    Nokia5110_DisplayBuffer();

    // Configure interrupt
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()); // 1 sec?

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);
}


void InitializeGrid(void)
{
    uint8_t i, j;

    Nokia5110_PrintBMP(0, 47, BMPMap, 0);

    for(i = 0; i < 41; i++)
        for(j = 0; j < 84; j++)
            if(Fruits[i][j] == 1)
                Nokia5110_DrawPixel(j, i + 7);

    Nokia5110_PrintBMP(0,  4, T, 0);            Nokia5110_PrintBMP(4,  4, I, 0);
    Nokia5110_PrintBMP(8,  4, M, 0);            Nokia5110_PrintBMP(12, 4, E, 0);
    Nokia5110_PrintBMP(16, 4, TWOPOINTS, 0);    Nokia5110_PrintBMP(44, 4, S, 0);
    Nokia5110_PrintBMP(48, 4, C, 0);            Nokia5110_PrintBMP(52, 4, O, 0);
    Nokia5110_PrintBMP(56, 4, R, 0);            Nokia5110_PrintBMP(60, 4, E, 0);
    Nokia5110_PrintBMP(64, 4, TWOPOINTS, 0);

    Nokia5110_PrintBMP(68, 4, Numbers[Score / 1000], 0);
    Nokia5110_PrintBMP(72, 4, Numbers[(Score % 1000) / 100], 0);
    Nokia5110_PrintBMP(76, 4, Numbers[((Score % 1000) % 100) / 10], 0);
    Nokia5110_PrintBMP(80, 4, Numbers[((Score % 1000) % 100) % 10], 0);

    Nokia5110_PrintBMP(20, 4, Numbers[Time / 1000], 0);
    Nokia5110_PrintBMP(24, 4, Numbers[(Time % 1000) / 100], 0);
    Nokia5110_PrintBMP(28, 4, Numbers[((Time % 1000) % 100) / 10], 0);
    Nokia5110_PrintBMP(32, 4, Numbers[((Time % 1000) % 100) % 10], 0);
}


void Move(void)
{
    CheckEndGame();

    // Button check
    switch(ConvertSwitch(GetButton()))
    {
        case PACMAN_RIGHT:
            PacMan.direction = PACMAN_RIGHT;
            break;

        case PACMAN_LEFT:
            PacMan.direction = PACMAN_LEFT;
            break;

        case PACMAN_UP:
            PacMan.direction = PACMAN_UP;
            break;

        case PACMAN_DOWN:
            PacMan.direction = PACMAN_DOWN;
            break;

        case SW_OK:
            Pause();
            InitializeGrid();
            SysCtlDelay(SysCtlClockGet() / 10);
            break;

        default:
            break;
    }

    switch(PacMan.direction)
    {
        case PACMAN_RIGHT:
            if(CheckMovement(PacMan.xPos + 1, PacMan.yPos))
            {
                EraseCharacter();
                PacMan.xPos = PacMan.xPos + 1;
                EatFruit(PacMan.xPos, PacMan.yPos);
            }

            break;

        case PACMAN_LEFT:
            if(CheckMovement(PacMan.xPos - 1, PacMan.yPos))
            {
                EraseCharacter();
                PacMan.xPos = PacMan.xPos - 1;
                EatFruit(PacMan.xPos, PacMan.yPos);
            }

            break;

        case PACMAN_UP:
            if(CheckMovement(PacMan.xPos, PacMan.yPos - 1))
            {
                EraseCharacter();
                PacMan.yPos = PacMan.yPos - 1;
                EatFruit(PacMan.xPos, PacMan.yPos);
            }

            break;

        case PACMAN_DOWN:
            if(CheckMovement(PacMan.xPos, PacMan.yPos + 1))
            {
                EraseCharacter();
                PacMan.yPos = PacMan.yPos + 1;
                EatFruit(PacMan.xPos, PacMan.yPos);
            }
            
            break;

        default:
            break;
    }

    DrawCharacter();
    Nokia5110_DisplayBuffer();
}


void DrawCharacter(void)
{
    Nokia5110_PrintBMP(PacMan.xPos, PacMan.yPos, PacMan.BMP[ PacMan.direction ], 0);
    Nokia5110_DisplayBuffer();
}


void EraseCharacter(void)
{
    Nokia5110_ClearBitmap(PacMan.xPos, PacMan.yPos, PacMan.BMP[ PacMan.direction ]);
    Nokia5110_DisplayBuffer();
}


bool CheckMovement(uint8_t C, uint8_t L) // Recebemos (x, y), onde x é a coluna e y a linha
{
    // Mapa[Linha][Coluna]
    return (Map[L - 7 - 2][C + 2] == 1) && (Map[L - 7 - 2][C + 3] == 1) &&
           (Map[L - 7 - 3][C + 2] == 1) && (Map[L - 7 - 3][C + 3] == 1)  ? true : false;
}


void EatFruit(uint8_t C, uint8_t L)
{
    // Fruits[Linha][Coluna]
    if((Fruits[L - 7 - 2][C + 2] == 1) || (Fruits[L - 7 - 2][C + 3] == 1) ||
       (Fruits[L - 7 - 3][C + 2] == 1) || (Fruits[L - 7 - 3][C + 3] == 1))
    {
        if(Fruits[L - 7 - 2][C + 2] == 1)   Fruits[L - 7 - 2][C + 2] = 2;
        if(Fruits[L - 7 - 2][C + 3] == 1)   Fruits[L - 7 - 2][C + 3] = 2;
        if(Fruits[L - 7 - 3][C + 2] == 1)   Fruits[L - 7 - 3][C + 2] = 2;
        if(Fruits[L - 7 - 3][C + 3] == 1)   Fruits[L - 7 - 3][C + 3] = 2;

        Nokia5110_ClearSquare(C + 2, L - 2, 2); // Aqui não diminui 7, pois é na tela.

        Score = Score + 1;
    }

    Nokia5110_PrintBMP(68, 4, Numbers[Score / 1000], 0);
    Nokia5110_PrintBMP(72, 4, Numbers[(Score % 1000) / 100], 0);
    Nokia5110_PrintBMP(76, 4, Numbers[((Score % 1000) % 100) / 10], 0);
    Nokia5110_PrintBMP(80, 4, Numbers[((Score % 1000) % 100) % 10], 0);
}


uint8_t ConvertSwitch(uint8_t tmp)
{
    switch(tmp)
    {
        case 12:    return PACMAN_UP;
        case 21:    return PACMAN_LEFT;
        case 22:    return SW_OK;
        case 23:    return PACMAN_RIGHT;
        case 32:    return PACMAN_DOWN;

        default:    break;
    }

    return BUTTON_NOT_PRESSED;
}


void CheckEndGame(void)
{
    if(Score >= 203 || Time >= 500) // Se comeu todas as frutas
    {
        GamePaused = true;
        Score = Score - (0.406 * Time);
        EndGame();
    }

    // Senão, se foi comido pelo sprite
    // PacMan.life = 0;
    // Nokia5110_PrintBMP(0, 47, BMPGameOver, 0);
    // SysCtlDelay(SysCtlClockGet());
    // EndGame();
}


void EndGame(void)
{
    uint8_t i, j;

    if(Score > BestScore)   BestScore = Score;
    Nokia5110_PrintBMP(0, 47, BMPBestScore, 0);

    Nokia5110_PrintBMP(35, 29, Numbers[BestScore / 1000], 0);
    Nokia5110_PrintBMP(39, 29, Numbers[(BestScore % 1000) / 100], 0);
    Nokia5110_PrintBMP(43, 29, Numbers[((BestScore % 1000) % 100) / 10], 0);
    Nokia5110_PrintBMP(47, 29, Numbers[((BestScore % 1000) % 100) % 10], 0);

    Nokia5110_DisplayBuffer();

    for(i = 0; i < 41; i++)
        for(j = 0; j < 84; j++)
            if(Fruits[i][j] == 2) // Frutas que foram comidas
                Fruits[i][j] = 1;

    SysCtlDelay(SysCtlClockGet() / 10);

    while(ConvertSwitch(GetButton()) == BUTTON_NOT_PRESSED){}

    Score = 0;
    Time = 0;

    PacMan.xPos = 2;
    PacMan.yPos = 14;
    PacMan.life = 1;
    PacMan.direction = PACMAN_RIGHT;

    Nokia5110_Clear();
    Nokia5110_ClearBuffer();

    InitializeGrid();

    EatFruit(PacMan.xPos, PacMan.yPos);
    Nokia5110_PrintBMP(PacMan.xPos, PacMan.yPos, PacMan.BMP[ PacMan.direction ], 0);

    Nokia5110_DisplayBuffer();

    GamePaused = false;
    // Jogo reiniciado
}
