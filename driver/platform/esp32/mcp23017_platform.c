// SPDX-License-Identifier: MIT

#include "mcp23017_platform.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Questa è la funzione che viene chiamata dopo un evento di interrupt.
 *
 * @param[in] arg Recupera il puntatore del dispositivo (passato al momento della registrazione).
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    mcp23017_isr_event_t *event = (mcp23017_isr_event_t *)arg;

    if (event->device->isr_callback)
        event->device->isr_callback(event);
}

/**
 * @brief Implementa la logica per leggere il buffer I2C tramite ESP32.
 *
 * @param[in]  handle               Dispositivo MCP23017.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[in]  length               Dimensione del buffer.
 * @param[out] data                 Buffer dei dati da ricevere.
 * @retval MCP23017_ERR_OK          Successo.
 * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
 * @retval MCP23017_ERR_FAIL        Errore durante la ricezione dei dati.
 * @retval MCP23017_ERR_TIMEOUT     Timeout raggiunto.
 */
static mcp23017_err_t i2c_read_register(void *handle, const uint8_t reg, uint8_t *data)
{
    // Verifica i parametri
    if (!handle || !data)
        return MCP23017_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Ottiene la struttura del dispositivo
    mcp23017_t *device = (mcp23017_t *)handle;

    // Invia il comando di scrittura e lettura nel bus I2C
    status = i2c_master_transmit_receive(
        (i2c_master_dev_handle_t)device->i2c_device_handle, &reg, 1, data, 1, device->i2c_timeout);

    if (status == ESP_ERR_TIMEOUT)
        return MCP23017_ERR_TIMEOUT;

    else if (status != ESP_OK)
        return MCP23017_ERR_FAIL;

    return MCP23017_ERR_OK;
}

/**
 * @brief Implementa la logica per scrivere il buffer I2C tramite ESP32.
 *
 * @param[in] handle                Dispositivo MCP23017.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] length                Dimensione del buffer.
 * @param[in] data                  Buffer dei dati da inviare.
 * @retval MCP23017_ERR_OK          Successo.
 * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
 * @retval MCP23017_ERR_FAIL        Errore durante la ricezione dei dati.
 * @retval MCP23017_ERR_TIMEOUT     Timeout raggiunto.
 */
static mcp23017_err_t i2c_write_register(void *handle, const uint8_t reg, const uint8_t data)
{
    // Verifica il parametro
    if (!handle)
        return MCP23017_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Ottiene la struttura del dispositivo
    mcp23017_t *device = (mcp23017_t *)handle;

    // Alloca un buffer statico di 2 byte
    const uint8_t buffer[2] = {reg, data};

    status = i2c_master_transmit(
        (i2c_master_dev_handle_t)device->i2c_device_handle, buffer, 2, device->i2c_timeout);

    if (status == ESP_ERR_TIMEOUT)
        return MCP23017_ERR_TIMEOUT;

    else if (status != ESP_OK)
        return MCP23017_ERR_FAIL;

    return MCP23017_ERR_OK;
}

static const mcp23017_platform_t mcp23017_platform = {
    .i2c_read = i2c_read_register,
    .i2c_write = i2c_write_register,
};

mcp23017_err_t mcp23017_init_hal(mcp23017_t *device,
    i2c_master_bus_handle_t bus_handle,
    const uint8_t i2c_address,
    const uint32_t i2c_scl_speed,
    const int32_t i2c_timeout)
{
    // Verifica i parametri
    if (!device || !bus_handle)
        return MCP23017_ERR_INVALID_ARG;

    if (i2c_address < 0x20 || i2c_address > 0x27)
        return MCP23017_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;
    i2c_master_dev_handle_t device_handle;

    // Configura l'handle I2C del dispositivo
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_address,
        .scl_speed_hz = i2c_scl_speed,
    };

    // Inizializza il dispositivo sul bus I2C
    status = i2c_master_bus_add_device(bus_handle, &i2c_device_config, &device_handle);

    if (status != ESP_OK)
        return MCP23017_ERR_FAIL;

    // Inizializza i campi restanti del dispositivo
    device->i2c_bus_handle = (void *)bus_handle;
    device->i2c_device_handle = (void *)device_handle;
    device->i2c_address = i2c_address;
    device->i2c_timeout = i2c_timeout;
    device->int_pin[0] = GPIO_NUM_NC;
    device->int_pin[1] = GPIO_NUM_NC;
    device->int_mode[0] = MCP23017_INT_BOTH;
    device->int_mode[1] = MCP23017_INT_BOTH;
    device->last_pin_state[0] = 0x00;
    device->last_pin_state[1] = 0x00;
    device->isr_callback = NULL;
    device->platform = &mcp23017_platform;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_hal_setup_int(mcp23017_t *device, int8_t int_pin_a, int8_t int_pin_b)
{
    // Verifica i parametri
    if (!device || int_pin_a > GPIO_NUM_MAX || int_pin_b > GPIO_NUM_MAX)
        return MCP23017_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    if (int_pin_a > GPIO_NUM_NC)
    {
        // Configurazione del pin INTA
        gpio_config_t int_pin_config_a = {
            .intr_type = GPIO_INTR_NEGEDGE,        // Imposta l'interrupt active-low
            .mode = GPIO_MODE_INPUT,               // Configura il pin come input
            .pin_bit_mask = (1ULL << int_pin_a),   // Valore del pin da configurare
            .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disabilita il pull-down interno sul pin
            .pull_up_en = GPIO_PULLUP_ENABLE,      // Abilita il pull-up interno sul pin
        };

        // Applica la configurazione al GPIO
        status = gpio_config(&int_pin_config_a);

        if (status != ESP_OK)
            return MCP23017_ERR_FAIL;

        // Aggiunge l'ISR handler al GPIO specificato
        status = gpio_isr_handler_add(device->int_pin[0], gpio_isr_handler, &device->isr_event[0]);

        if (status != ESP_OK)
            return MCP23017_ERR_FAIL;

        // Aggiorna i campi del dispositivo
        device->int_pin[0] = int_pin_a;
        device->isr_event[0].device = device;
        device->isr_event[0].port = MCP23017_PORT_A;
    }

    if (int_pin_b > GPIO_NUM_NC)
    {
        // Configurazione del pin INTB
        gpio_config_t int_pin_config_b = {
            .intr_type = GPIO_INTR_NEGEDGE,        // Imposta l'interrupt active-low
            .mode = GPIO_MODE_INPUT,               // Configura il pin come input
            .pin_bit_mask = (1ULL << int_pin_b),   // Valore del pin da configurare
            .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disabilita il pull-down interno sul pin
            .pull_up_en = GPIO_PULLUP_ENABLE,      // Disabilita il pull-up interno sul pin
        };

        // Applica la configurazione al GPIO
        status = gpio_config(&int_pin_config_b);

        if (status != ESP_OK)
            return MCP23017_ERR_FAIL;

        // Aggiunge l'ISR handler al GPIO specificato
        status = gpio_isr_handler_add(device->int_pin[1], gpio_isr_handler, &device->isr_event[1]);

        if (status != ESP_OK)
            return MCP23017_ERR_FAIL;

        // Aggiorna i campi del dispositivo
        device->int_pin[1] = int_pin_b;
        device->isr_event[1].device = device;
        device->isr_event[1].port = MCP23017_PORT_B;
    }

    return MCP23017_ERR_OK;
}
