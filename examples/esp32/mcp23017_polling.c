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

// Definisce il tag per il debugging
static const char *TAG = "main";

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

void i2c_bus_init(i2c_master_bus_handle_t *p_i2c_bus_handle)
{
    // Configura il bus I2C
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_PIN_SDA,
        .scl_io_num = I2C_PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // Disabilita il pull-up interno perché troppo debole
    };

    // Inizializza il bus I2C
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, p_i2c_bus_handle));

    ESP_LOGI(TAG, "I2C bus initialized successfully");
}

void app_main(void)
{
    // Handler del bus I2C
    i2c_master_bus_handle_t i2c_bus_handle = NULL;

    // Inizializza il bus I2C
    i2c_bus_init(&i2c_bus_handle);

    // Definisce e inizializza a zero il dispositivo
    mcp23017_t device = {0};

    // Inizializza il platform del dispositivo per comunicare con l'hardware
    mcp23017_init_hal(&device, i2c_bus_handle, I2C_ADDRESS, I2C_SCL_SPEED_HZ, I2C_TIMEOUT);

    // Inizializza il core del dispositivo con la configurazione specificata
    mcp23017_init_core(&device, 0b00000000);

    // ===============================================
    // Configura i pin A0 e A1 come pulsanti (porta A)
    // ===============================================

    // Abilita i pull-up dei pin
    mcp23017_set_pullup(&device, MCP23017_PORT_A, 0b11111111, 0b11111111);

    // Imposta i pin come input
    mcp23017_set_direction(&device, MCP23017_PORT_A, 0b00000011, 0b00000011);

    // ==============================
    // Esegue il polling dei pulsanti
    // ==============================

    uint8_t gpio = 0x00;

    for (;;)
    {
        // Ottiene lo stato logico dei pin della porta A
        mcp23017_get_gpio(&device, MCP23017_PORT_A, &gpio);
        print_reg_8("PORT A > PIN STATE", gpio);

        // Termina se entrambi i pin A0 e A1, sono premuti (active-low)
        if ((gpio & 0x03) == 0x00)
            break;

        // Ritardo di 100 ms per la lettura in console
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "BOTH BUTTONS PRESSED");

    // Rimuove il dispositivo MCP23017 dal bus I2C
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(device.i2c_device_handle));
    ESP_LOGI(TAG, "MCP23017 device removed successfully");

    // Libera il bus I2C
    ESP_ERROR_CHECK(i2c_del_master_bus(i2c_bus_handle));
    ESP_LOGI(TAG, "I2C bus successfully freed");
}
