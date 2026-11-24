/*
 * Minimal remote_bitbang firmware for Raspberry Pi Pico.
 * Implements OpenOCD's remote_bitbang serial protocol directly over
 * USB CDC (stdio), with plain synchronous GPIO bit-banging.
 * No PIO, no DMA -- intentionally dumb and robust.
 *
 * Protocol (one byte in, at most one byte out per command):
 *   '0'-'7' : set TCK=bit2, TMS=bit1, TDI=bit0
 *   'R'     : sample TDO, reply with '0' or '1'
 *   'r'-'u' : set TRST=bit1, SRST=bit0 (we only drive SRST via PIN_RST)
 *   'B'/'b' : LED on/off
 *   'Q'     : quit (no-op)
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define PIN_TDI 16
#define PIN_TDO 17
#define PIN_TCK 18
#define PIN_TMS 19
#define PIN_RST 20

int main(void)
{
    stdio_init_all();

    gpio_init(PIN_TCK);
    gpio_set_dir(PIN_TCK, GPIO_OUT);
    gpio_put(PIN_TCK, 0);

    gpio_init(PIN_TDI);
    gpio_set_dir(PIN_TDI, GPIO_OUT);
    gpio_put(PIN_TDI, 0);

    gpio_init(PIN_TMS);
    gpio_set_dir(PIN_TMS, GPIO_OUT);
    gpio_put(PIN_TMS, 1);

    gpio_init(PIN_TDO);
    gpio_set_dir(PIN_TDO, GPIO_IN);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1); /* idle: SRST not asserted (active low) */

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

    while (true) {
        int c = getchar();
        if (c == PICO_ERROR_TIMEOUT)
            continue;

        if (c >= '0' && c <= '7') {
            int v = c - '0';
            gpio_put(PIN_TDI, v & 0x1);
            gpio_put(PIN_TMS, (v >> 1) & 0x1);
            gpio_put(PIN_TCK, (v >> 2) & 0x1);
        } else if (c == 'R') {
            putchar(gpio_get(PIN_TDO) ? '1' : '0');
            fflush(stdout);
        } else if (c >= 'r' && c <= 'u') {
            int v = c - 'r';
            int srst = v & 0x1;
            gpio_put(PIN_RST, srst ? 0 : 1);
        }
#ifdef PICO_DEFAULT_LED_PIN
        else if (c == 'B') {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
        } else if (c == 'b') {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        }
#endif
        else if (c == 'Q') {
            /* quit: no-op, host closes the connection */
        }
    }
}
