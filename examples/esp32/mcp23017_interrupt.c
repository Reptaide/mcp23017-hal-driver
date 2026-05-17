// SPDX-License-Identifier: MIT

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "mcp23017_core.h"
#include "mcp23017_platform.h"

#include <stdio.h>

#define I2C_ADDRESS 0x27        // Indirizzo di default
#define I2C_PORT I2C_NUM_0      // Numero porta I2C
#define I2C_PIN_SCL 10          // SCL
#define I2C_PIN_SDA 11          // SDA
#define I2C_SCL_SPEED_HZ 100000 // 100 kHz
#define I2C_TIMEOUT 200         // Timeout I2C in ms
#define INT_A_PIN 13            // Interrupt porta A
#define INT_B_PIN 14            // Interrupt porta B

// Definisce il tag per il debugging
static const char *TAG = "main";
static TaskHandle_t mcp23017_task_handle = NULL;
// globale condiviso tra main.c e platform.c
static QueueHandle_t mcp23017_event_queue = NULL;

/**
 * @brief Funzione di supporto per visualizzare lo stato dei bit.
 *
 * @param[in] name  Nome per identificare il registro nella console.
 * @param[in] value Valore del registro da visualizzare.
 */
static void print_reg_8(const char *name, const uint8_t value)
{
    ESP_LOGI(TAG,
        "%s (D[7:0]): %u %u %u %u %u %u %u %u",
        name,
        (value >> 7) & 0x01,
        (value >> 6) & 0x01,
        (value >> 5) & 0x01,
        (value >> 4) & 0x01,
        (value >> 3) & 0x01,
        (value >> 2) & 0x01,
        (value >> 1) & 0x01,
        (value >> 0) & 0x01);
}

/**
 * @brief Inizializza e configura il bus I2C.
 *
 * @param[in] i2c_bus_handle Handle del bus I2C.
 */
void i2c_bus_init(i2c_master_bus_handle_t *i2c_bus_handle)
{
    // Configura il bus I2C
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_PIN_SDA,
        .scl_io_num = I2C_PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    // Inizializza il bus I2C
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, i2c_bus_handle));

    ESP_LOGI(TAG, "I2C bus initialized successfully");
}

/**
 * @brief Task di elaborazione asincrono fuori dal contesto di interrupt che legge i risultati del
 * dispositivo che ha generato l'interrupt.
 *
 * @param[in] arg Argomento passato al task.
 */
static void mcp23017_task(void *arg)
{
    mcp23017_t *device = (mcp23017_t *)arg;
    mcp23017_port_t port;
    uint8_t events_a, state_a;
    uint8_t events_b, state_b;

    for (;;)
    {
        // Attende un evento dalla coda
        if (xQueueReceive(mcp23017_event_queue, &port, portMAX_DELAY) == pdFALSE)
            continue;

        for (;;)
        {
            mcp23017_get_int_event(device, MCP23017_PORT_A, &events_a, &state_a);
            mcp23017_get_int_event(device, MCP23017_PORT_B, &events_b, &state_b);

            if (events_a)
            {
                print_reg_8("PORT A EVENTS", events_a);
                print_reg_8("PORT A STATE", state_a);
            }

            if (events_b)
            {
                print_reg_8("PORT B EVENTS", events_b);
                print_reg_8("PORT B STATE", state_b);
            }
        }
    }
}

/**
 * @brief Questa funzione serve a notificare un task quando avviene un interrupt hardware.
 *
 * @param[in] context Contiene il puntatore al TaskHandle_t
 */
static void IRAM_ATTR mcp23017_isr_callback(mcp23017_isr_event_t *event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(mcp23017_event_queue, &event->port, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void app_main(void)
{
    // Handler del bus I2C
    i2c_master_bus_handle_t i2c_bus_handle = NULL;

    // Inizializza il bus I2C
    i2c_bus_init(&i2c_bus_handle);

    // Definisce e inizializza a zero il dispositivo
    mcp23017_t device = {0};

    // Inizializza il platform per comunicare con l'hardware
    mcp23017_init_hal(&device, i2c_bus_handle, I2C_ADDRESS, I2C_SCL_SPEED_HZ, I2C_TIMEOUT);

    // Config 0b01000000, un solo pin di interrupt per entrambe le porte (mirror = 1)
    // Config 0b00000000, due pin di interrupt ognuno per una porta (mirror = 0)
    uint8_t config = 0b00000000;
    mcp23017_init_core(&device, config);

    mcp23017_get_config(&device, &config);
    print_reg_8("DEVICE CONFIG", config);

    // Installa nella IRAM il servizio di interrupt GPIO globale
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    // Crea la coda per accogliere gli interrupt
    mcp23017_event_queue = xQueueCreate(10, sizeof(mcp23017_port_t));

    // Crea il task per gestire l'evento del sensore
    xTaskCreate(mcp23017_task, "mcp23017_task", 4096, &device, 5, &mcp23017_task_handle);

    // Registra la callback dell'interrupt (chiamare prima di mcp23017_hal_setup_int)
    mcp23017_register_isr_callback(&device, mcp23017_isr_callback);

    // Configura i pin di interrupt per entrambe le porte
    mcp23017_hal_setup_int(&device, INT_A_PIN, INT_B_PIN);

    // ===================================================
    // Configura i pin A0, A1 e A2 come pulsanti (porta A)
    // ===================================================

    // Abilita i pull-up dei pin
    mcp23017_set_pullup(&device, MCP23017_PORT_A, 0b00000111, 0b00000111);

    // Imposta i pin come input
    mcp23017_set_direction(&device, MCP23017_PORT_A, 0b00000111, 0b00000111);

    // Abilita gli interrupt (active-low) sui pin
    mcp23017_attach_int(&device, MCP23017_PORT_A, 0b00000111, MCP23017_INT_FALLING);

    // ===================================================
    // Configura i pin B0, B1 e B2 come pulsanti (porta B)
    // ===================================================

    // Abilita i pull-up dei pin
    mcp23017_set_pullup(&device, MCP23017_PORT_B, 0b00000111, 0b00000111);

    // Imposta i pin come input
    mcp23017_set_direction(&device, MCP23017_PORT_B, 0b00000111, 0b00000111);

    // Abilita gli interrupt (active-low) sui pin
    mcp23017_attach_int(&device, MCP23017_PORT_B, 0b00000111, MCP23017_INT_FALLING);

    // Loop infinito, previene la chiusura del programma
    for (;;)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
