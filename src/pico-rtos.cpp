#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "queue.h"

using namespace std;

QueueHandle_t inputQueue;

void user_input_task(void *pvParameters) {
    char receivedChar;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        // Wait for a character to be placed in the queue
        if (xQueueReceive(inputQueue, &receivedChar, portMAX_DELAY) == pdTRUE) {
            // Process the received character
            printf("Received character: %c\n", receivedChar);
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(50));

            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// Simulate receiving input data, which would typically be done in an interrupt
void simulate_input_task(void *pvParameters) {
    const char *testString = "Hello FreeRTOS\n";
    while (true) {
        // Simulate receiving input character by character
        for (const char *p = testString; *p != '\0'; ++p) {
            // Send the character to the input queue
            xQueueSend(inputQueue, p, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(100)); // Delay between characters for demonstration
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay before simulating the next input
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


void serial_task(void *pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // Frequency of 100 ms

    xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Get the current tick count
        TickType_t currentTick = xTaskGetTickCount();

        // Convert ticks to milliseconds
        uint32_t timeInMs = currentTick * portTICK_PERIOD_MS;
        uint32_t seconds = timeInMs / 1000;
        uint32_t milliseconds = timeInMs % 1000;

        // Print the current time in seconds and milliseconds
        printf("Time: %lu.%03lu seconds - Serial Test\n", seconds, milliseconds);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


void setup() {
    // Add any setup code here, if needed
}

int main() {
    stdio_init_all(); // Initialize standard I/O

    setup();

    inputQueue = xQueueCreate(10, sizeof(char));

    if (inputQueue == NULL) {
        printf("Failed to create input queue\n");
        while (1);
    }

    // Create the user input task
    // if (xTaskCreate(user_input_task, "User Input Task", 256, NULL, 4, NULL) != pdPASS) {
    //     printf("Failed to create user input task\n");
    //     while (1);
    // }
    //
    // // Create the simulated input task
    // if (xTaskCreate(simulate_input_task, "Simulate Input Task", 256, NULL, 3, NULL) != pdPASS) {
    //     printf("Failed to create simulate input task\n");
    //     while (1);
    // }

    // Create LED task
    if (xTaskCreate(led_task, "led_task", 256, NULL, 1, NULL) != pdPASS) {
        printf("Failed to create LED task\n");
        while (1); // Halt if task creation failed
    }

    if (xTaskCreate(serial_task, "serial_task", 256, NULL, 2, NULL) != pdPASS) {
        printf("Failed to create LED task\n");
        while (1); // Halt if task creation failed
    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    printf("Scheduler failed to start\n");
    while (1);
}
