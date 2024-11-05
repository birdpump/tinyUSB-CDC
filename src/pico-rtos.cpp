#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "queue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/uart.h"
#include "telemetry.pb.h"  // Generated from protobuf
#include "pb_encode.h"
#include "pb_decode.h"

#define START_BYTE 0xAA
#define TELEMETRY_TYPE 0x01
#define COMMAND_TYPE 0x02
#define UART_ID uart0
#define BAUD_RATE 115200
#define TX_PIN PICO_DEFAULT_UART_TX_PIN
#define RX_PIN PICO_DEFAULT_UART_RX_PIN

uint8_t calculateChecksum(const uint8_t* buffer, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    return checksum;
}

void sendTelemetryTask(void* pvParameters) {
    while (true) {
        Telemetry telemetry = Telemetry_init_zero;
        telemetry.temperature = 23.5;
        telemetry.humidity = 50.0;

        uint8_t buffer[64];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        pb_encode(&stream, Telemetry_fields, &telemetry);
        size_t message_size = stream.bytes_written;

        uart_putc(UART_ID, START_BYTE);
        uart_putc(UART_ID, message_size);
        uart_putc(UART_ID, TELEMETRY_TYPE);
        uart_write_blocking(UART_ID, buffer, message_size);
        uart_putc(UART_ID, calculateChecksum(buffer, message_size));

        vTaskDelay(pdMS_TO_TICKS(500));  // Send every 1 second
    }
}

void commandListenerTask(void* pvParameters) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    while (true) {
        if (uart_is_readable(UART_ID)) {
            if (uart_getc(UART_ID) == START_BYTE) {
                uint8_t length = uart_getc(UART_ID);
                uint8_t type = uart_getc(UART_ID);
                uint8_t buffer[64];
                uart_read_blocking(UART_ID, buffer, length);

                uint8_t received_checksum = uart_getc(UART_ID);
                if (received_checksum == calculateChecksum(buffer, length) && type == COMMAND_TYPE) {
                    Command command = Command_init_zero;
                    pb_istream_t stream = pb_istream_from_buffer(buffer, length);
                    pb_decode(&stream, Command_fields, &command);

                    if (command.test) {
                        gpio_put(PICO_DEFAULT_LED_PIN, 1);
                    } else {
                        gpio_put(PICO_DEFAULT_LED_PIN, 0);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Prevents tight loop
    }
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    xTaskCreate(sendTelemetryTask, "Send Telemetry Task", 256, NULL, 1, NULL);
    xTaskCreate(commandListenerTask, "Command Listener Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true);
}
