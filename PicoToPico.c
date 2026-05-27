#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"

#include "blink.pio.h"

// void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
//     blink_program_init(pio, sm, offset, pin);
//     pio_sm_set_enabled(pio, sm, true);

//     printf("Blinking pin %d at %d Hz\n", pin, freq);

//     // PIO counter program takes 3 more cycles in total than we pass as
//     // input (wait for n + 1; mov; jmp)
//     pio->txf[sm] = (125000000 / (2 * freq)) - 3;
// }

int comm_check() {
    bool readReady = false;

    while (readReady == false) {
        printf("Awaiting read ready...\n");

        // char readFromUartC = uart_getc(UART_ID);

        // if (readFromUartC == 'r') {
        //     readReady == true;
        // }

        int readFromUartC = getchar_timeout_us(0);
        if (readFromUartC != PICO_ERROR_TIMEOUT) {
            printf("Read ready!");
            readReady = true;
        }

        sleep_ms(100);
    }

    return 1;
}

int comm_send() {

}

int comm_receive() {

}

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5



int main() {
    stdio_init_all();

    // PIO Blinking example
    // PIO pio = pio0;
    // uint offset = pio_add_program(pio, &blink_program);
    // printf("Loaded program at %d\n", offset);
    
    // #ifdef PICO_DEFAULT_LED_PIN
    // blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    // #else
    // blink_pin_forever(pio, 0, offset, 6, 3);
    // #endif

    while(true) {
        int awaitEnter = getchar_timeout_us(0);
        if (awaitEnter != PICO_ERROR_TIMEOUT) {
            if (awaitEnter == 10 || awaitEnter == 13) {
                break;
            }
        }
    }

    // Set up our UART
    int baudrate = 115200;
    char buffer[128];
    printf("Input new baudrate or keep old by pressing [Enter]: ");
    while(true) {
        int tempBaudRate = 0;
        int readBaudRate = getchar_timeout_us(0);
        if (readBaudRate != PICO_ERROR_TIMEOUT) {
            if (scanf("%s", buffer) == 1) {
                printf("You sent: %s\n", buffer);
                tempBaudRate = atoi(buffer);
            }

            if (readBaudRate == 10 || readBaudRate == 13) {
                baudrate = tempBaudRate;
                break;
            }
        }
    }

    printf("New baudrate: %d\n", baudrate);

    // uart_init(UART_ID, baudrate);
    
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // uart_puts(UART_ID, " Hello, UART!\n");

    int commReady = 0;
    commReady = comm_check();

    while (commReady == 1) {
        printf("Hello, Skylink!\n");
        sleep_ms(100);
    }
}