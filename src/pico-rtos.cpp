#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"

#include "tusb.h"
#include "queue.h"

using namespace std;

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
                // Process the received data in `buffer`
                // For example, send it to another queue for further processing
                // xQueueSendToBack(...);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay to prevent tight loop
    }
}

// FreeRTOS task for sending data to the USB host
void usb_send_task(void *pvParameters) {
    uint8_t buffer[BULK_EP_SIZE];

    while (true) {
        if (tud_vendor_write_available()) {
            // Check if there’s data to send from the queue
            if (xQueueReceive(sendQueue, buffer, pdMS_TO_TICKS(10)) == pdPASS) {
                tud_vendor_write(buffer, sizeof(buffer));
                tud_vendor_write_flush();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay to prevent tight loop
    }
}


void led_task(void *pvParameters) {
    // Configure LED pin for output
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
    // Add any setup code here, if needed
}

int main() {
    stdio_init_all(); // Initialize standard I/O

    setup();


    stdio_init_all();
    tusb_init();

    // Create FreeRTOS queue
    sendQueue = xQueueCreate(10, BULK_EP_SIZE);

    // Create FreeRTOS tasks
    xTaskCreate(usb_receive_task, "USB Receive Task", 1024, NULL, 2, NULL);
    xTaskCreate(usb_send_task, "USB Send Task", 1024, NULL, 2, NULL);

    // Create LED task
    if (xTaskCreate(led_task, "led_task", 256, NULL, 1, NULL) != pdPASS) {
        printf("Failed to create LED task\n");
        while (1); // Halt if task creation failed
    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();
    // Should never reach here
    printf("Scheduler failed to start\n");
    while (1);
}
