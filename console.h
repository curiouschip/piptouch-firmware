#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

#define CONSOLE_PORT              Serial
#define CONSOLE_BAUD_RATE         115200
#define CONSOLE_CMD_BUFFER_SIZE   64

void console_init();
void console_tick();

#endif
