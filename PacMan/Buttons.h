#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>

#define BUTTON_NOT_PRESSED (99)

void    ConfigureButtons();
uint8_t GetButton();

#endif
