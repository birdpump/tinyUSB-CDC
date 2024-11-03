#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "queue.h"

#define BULK_IN_EP  0x81 // Pico to Host (IN)
#define BULK_OUT_EP 0x02 // Host to Pico (OUT)
#define BULK_EP_SIZE 64  // Packet size

QueueHandle_t sendQueue; // Queue for outgoing data

// FreeRTOS task for receiving data from the USB host
void usb_receive_task(void *pvParameters) {
    uint8_t buffer[BULK_EP_SIZE];

    while (true) {
        if (tud_vendor_available()) {
            // Read data from host
            uint32_t count = tud_vendor_read(buffer, sizeof(buffer));
            if (count > 0) {
//                // Print received data as ASCII
//                printf("Received data from host: ");
//                for (uint32_t i = 0; i < count; i++) {
//                    putchar(buffer[i]);
//                }
//                putchar('\n');
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay to prevent tight loop
    }
}

// FreeRTOS task for sending data to the USB host
void usb_send_task(void *pvParameters) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    uint8_t buffer[BULK_EP_SIZE];
    const char *message = "Hello from Pico!";

    while (true) {
        if (tud_vendor_write_available()) {
            // Copy message to buffer
            snprintf((char*)buffer, BULK_EP_SIZE, "%s", message);

            // Send buffer to the queue
            if (xQueueSendToBack(sendQueue, buffer, pdMS_TO_TICKS(10)) == pdPASS) {
                // Send data from the queue
                tud_vendor_write(buffer, sizeof(buffer));
                vTaskDelay(pdMS_TO_TICKS(800));
                tud_vendor_write_flush();
            }
        }

        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
        vTaskDelay(pdMS_TO_TICKS(1000)); // Send every second
    }
}

// Task to handle LED blinking
void led_task(void *pvParameters) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void setup() {
    // Any setup code here, if needed
}

int main() {
    stdio_init_all(); // Initialize standard I/O
    tusb_init(); // TinyUSB initialization

    setup();

    // Create FreeRTOS queue
    sendQueue = xQueueCreate(10, BULK_EP_SIZE);

    // Create FreeRTOS tasks
//    xTaskCreate(usb_receive_task, "USB Receive Task", 1024, NULL, 2, NULL);
    xTaskCreate(usb_send_task, "USB Send Task", 1024, NULL, 2, NULL);

    // Create LED task
//    if (xTaskCreate(led_task, "LED Task", 256, NULL, 1, NULL) != pdPASS) {
//        printf("Failed to create LED task\n");
//        while (1); // Halt if task creation failed
//    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    printf("Scheduler failed to start\n");
    while (1);
}
