#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "blink.pio.h"

// UART defines
#define UART_ID uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define DEFAULT_BAUDRATE 115200

// Mode: 0 = Send/Receive, 1 = Send only, 2 = Receive only
#define MODE 0

// Buffers for inter-core communication
#define BUFFER_SIZE 256
char tx_buffer[BUFFER_SIZE];
char rx_buffer[BUFFER_SIZE];
volatile int tx_ready = 0;
volatile int rx_ready = 0;
volatile uint32_t uart_baudrate = DEFAULT_BAUDRATE;

// Function to get baudrate from user
uint32_t get_baudrate_from_user() {
    char input[16];
    uint32_t baudrate = DEFAULT_BAUDRATE;

    printf("Enter baudrate (or press Enter for %d): ", DEFAULT_BAUDRATE);
    fflush(stdout);

    int awaitEnter = getchar_timeout_us(0);
        if (awaitEnter != PICO_ERROR_TIMEOUT) {
            if (awaitEnter == 10 || awaitEnter == 13) {
                printf("Using default: %d\n", DEFAULT_BAUDRATE);
                return DEFAULT_BAUDRATE;
            }
        }

    // if (fgets(input, sizeof(input), stdin) != NULL) {
    //     // Check if user just pressed Enter (empty input)
    //     if (input[0] == 10 || input[0] == 13) {
    //         printf("Using default: %d\n", DEFAULT_BAUDRATE);
    //         return DEFAULT_BAUDRATE;
    //     }

    //     int awaitEnter = getchar_timeout_us(0);
    //     if (awaitEnter != PICO_ERROR_TIMEOUT) {
    //         if (awaitEnter == 10 || awaitEnter == 13) {
    //             printf("Using default: %d\n", DEFAULT_BAUDRATE);
    //             return DEFAULT_BAUDRATE;
    //         }
    //     }

    //     // Parse the input
    //     char *endptr;
    //     baudrate = strtoul(input, &endptr, 10);

    //     // Validate the baudrate (common range)
    //     if (baudrate < 300 || baudrate > 921600) {
    //         printf("Invalid baudrate. Using default: %d\n", DEFAULT_BAUDRATE);
    //         return DEFAULT_BAUDRATE;
    //     }

    //     printf("Baudrate set to: %d\n", baudrate);
    // }

    return baudrate;
}

// Core 1: Handles UART RX/TX to PICO2
void core1_main() {
    // Set up UART (GPIO FIRST, then uart_init)
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_init(UART_ID, uart_baudrate);

    printf("[Core 1] UART initialized at %d baud\n", uart_baudrate);

    int rx_index = 0;

    while (true) {
        // Task 1: Send data from PC (via Core 0) to PICO2
        if (tx_ready) {
            uart_puts(UART_ID, tx_buffer);
            printf("[Core 1] Sent to PICO2: %s", tx_buffer);
            tx_ready = 0;
        }

        // Task 2: Receive data from PICO2 and buffer it
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            
            if (c == '\n' || c == '\r') {
                if (rx_index > 0) {
                    rx_buffer[rx_index] = '\0';
                    rx_ready = 1;  // Signal Core 0 that data is ready
                    printf("[Core 1] Received from PICO2: %s\n", rx_buffer);
                    rx_index = 0;
                }
            } else if (rx_index < BUFFER_SIZE - 1) {
                rx_buffer[rx_index++] = c;
            }
        }

        sleep_ms(10);
    }
}

// Core 0: Handles PC communication
int main() {
    stdio_init_all();

    printf("[Core 0] Starting...\n");

    // Wait for Enter to start
    printf("Press Enter to begin...\n");
    while(true) {
        int awaitEnter = getchar_timeout_us(0);
        if (awaitEnter != PICO_ERROR_TIMEOUT) {
            if (awaitEnter == 10 || awaitEnter == 13) {
                break;
            }
        }
    }

    // Get baudrate from user
    uart_baudrate = get_baudrate_from_user();

    // Launch Core 1 (after baudrate is set)
    multicore_launch_core1(core1_main);
    printf("[Core 0] Core 1 launched\n");

    printf("Mode: %d (0=Send+Receive, 1=Send Only, 2=Receive Only)\n", MODE);
    printf("Enter messages to send to PICO2:\n");

    while (true) {
        // Task 1: Read from PC (stdin)
        if ((MODE == 0 || MODE == 1) && getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
            // User started typing, read the full line
            if (fgets(tx_buffer, BUFFER_SIZE, stdin) != NULL) {
                tx_ready = 1;  // Signal Core 1 to send
                
                // Wait for Core 1 to process
                while (tx_ready) {
                    sleep_ms(1);
                }
            }
        }

        // Task 2: Display received data from PICO2
        if ((MODE == 0 || MODE == 2) && rx_ready) {
            printf("[Core 0] Data from PICO2: %s\n", rx_buffer);
            rx_ready = 0;
        }

        sleep_ms(10);
    }

    return 0;
}