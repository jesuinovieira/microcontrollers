#include "Buttons.h"

#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#define WRITE_REG(x)                      (*((volatile uint32_t *)(x)))

void ConfigureButtons()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    SysCtlDelay(SysCtlClockGet() / 4);

    WRITE_REG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    WRITE_REG(GPIO_PORTF_BASE + GPIO_O_CR)   = 0x01;

    // Linhas com output e colunas como input para fechar o circuíto
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5); // Segmentation Fault aqui
}


uint8_t GetColumn()
{
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == GPIO_PIN_0)    return 1;
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) == GPIO_PIN_1)    return 2;
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2) == GPIO_PIN_2)    return 3;
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) == GPIO_PIN_3)    return 4;

    return 0;
}


uint8_t GetButton()
{
    uint8_t i;
    uint8_t Column;

    for(i = 0; i < 4; i++)
    {
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 , 0x00);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0x00);

        // Ativa, verifica e desliga cada linha
        switch(i)
        {
            case 0:
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4 , 0xFF);

                if((Column = GetColumn()) != 0)
                    return (10 + Column);

                break;

            case 1:
                GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 , 0xFF);

                if((Column = GetColumn()) != 0)
                    return (20 + Column);

                break;

            case 2:
                GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1 , 0xFF);

                if((Column = GetColumn()) != 0)
                    return (30 + Column);

                break;

            case 3:
                GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5 , 0xFF);

                if((Column = GetColumn()) != 0)
                    return (40 + Column);

                break;
        }
    }

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 , 0x00);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0x00);

    return BUTTON_NOT_PRESSED;
}
