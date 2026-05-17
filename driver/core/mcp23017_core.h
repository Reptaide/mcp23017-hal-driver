/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2026 Reptaide.
 *
 * Released under the terms of the MIT License.
 * See LICENSE in the root of this repository for the full license text.
 */

#ifndef MCP23017_CORE_H
#define MCP23017_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct mcp23017_t mcp23017_t;
    typedef struct mcp23017_isr_event_t mcp23017_isr_event_t;

    /**
     * @brief Definisce i possibili errori restituiti dalle funzioni.
     */
    typedef enum
    {
        MCP23017_ERR_OK = 0,
        MCP23017_ERR_INVALID_ARG,
        MCP23017_ERR_FAIL,
        MCP23017_ERR_TIMEOUT,
    } mcp23017_err_t;

    /**
     * @brief Definisce i valori per le porte di interrupt.
     */
    typedef enum
    {
        MCP23017_PORT_A = 0,
        MCP23017_PORT_B = 1,
    } mcp23017_port_t;

    /**
     * @brief Definisce le modalità di trigger dell'interrupt.
     */
    typedef enum
    {
        MCP23017_INT_BOTH = 0,
        MCP23017_INT_FALLING,
        MCP23017_INT_RISING
    } mcp23017_int_mode_t;

    /**
     * @brief Contiene i campi dei registri per le porte A e B.
     */
    typedef struct
    {
        uint8_t reg_iodir_a;
        uint8_t reg_iodir_b;
        uint8_t reg_ipol_a;
        uint8_t reg_ipol_b;
        uint8_t reg_gpinten_a;
        uint8_t reg_gpinten_b;
        uint8_t reg_defval_a;
        uint8_t reg_defval_b;
        uint8_t reg_intcon_a;
        uint8_t reg_intcon_b;
        uint8_t reg_iocon_a;
        uint8_t reg_iocon_b;
        uint8_t reg_gppu_a;
        uint8_t reg_gppu_b;
        uint8_t reg_intf_a;
        uint8_t reg_intf_b;
        uint8_t reg_intcap_a;
        uint8_t reg_intcap_b;
        uint8_t reg_gpio_a;
        uint8_t reg_gpio_b;
        uint8_t reg_olat_a;
        uint8_t reg_olat_b;
    } mcp23017_bank_t;

    /**
     * @brief Il seguente codice definisce un modello (template) di firma per le seguenti funzioni.
     * L'implementazione di queste funzioni spetta allo sviluppatore in base al tipo di platform.
     */
    typedef mcp23017_err_t (*mcp23017_read_register_t)(
        void *handle, const uint8_t reg, uint8_t *data);
    typedef mcp23017_err_t (*mcp23017_write_register_t)(
        void *handle, const uint8_t reg, const uint8_t data);
    typedef void (*mcp23017_isr_callback_t)(mcp23017_isr_event_t *event);

    /**
     * @brief Contiene la definizione delle funzioni per comunicare con l'hardware. La
     * loro implementazione è a carico dello sviluppatore in base all'hardware selezionato.
     */
    typedef struct
    {
        mcp23017_read_register_t i2c_read;
        mcp23017_write_register_t i2c_write;
    } mcp23017_platform_t;

    /**
     * @brief Contiene la struttura dell'evento di interrupt, composto da:
     * - Un riferimento al dispositivo;
     * - Il numero di porta che ha generato l'interupt.
     */
    struct mcp23017_isr_event_t
    {
        mcp23017_t *device;
        mcp23017_port_t port;
    };

    /**
     * @brief Contiene la struttura del dispositivo.
     */
    struct mcp23017_t
    {
        void *i2c_bus_handle;                 // Handler del bus I2C
        void *i2c_device_handle;              // Handler dispositivo I2C
        uint8_t i2c_address;                  // Indirizzo I2C del dispositivo
        int32_t i2c_timeout;                  // Timeout I2C del dispositivo
        int8_t int_pin[2];                    // Pin di interrupt per le porte A/B
        uint8_t int_mode[2];                  // Modalità di interrupt per le porte A/B
        uint8_t last_pin_state[2];            // Ultimo INTCAP dei pin per le porte A/B
        mcp23017_isr_event_t isr_event[2];    // Lista degli eventi di interrupt per ogni porta
        mcp23017_isr_callback_t isr_callback; // Funzione chiamata all'arrivo di un interrupt
        const mcp23017_bank_t *bank;          // Puntatore alla mappa dei registri
        const mcp23017_platform_t *platform;  // Puntatore alla struttura platform
    };

    /**
     * @brief Inizializza il core del dispositivo MCP23017. I GPIO GPA7 e GPB7 non devono essere
     * utilizzati come GPIO di input perché possono interferire con il GPIO SDA del protocollo I2C.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] config                Valore per la configurazione del dispositivo.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_init_core(mcp23017_t *device, const uint8_t config);

    /**
     * @brief Restituisce lo stato della configurazione del dispositivo.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[out] value                Bitmask dei bit della configurazione.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_config(mcp23017_t *device, uint8_t *value);

    /**
     * @brief Applica la configurazione al dispositivo.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] bitmask               Bitmask dei bit da modificare.
     * @param[in] value                 Bitmask dello stato logico dei bit della configurazione.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_set_config(
        mcp23017_t *device, const uint8_t bitmask, const uint8_t value);

    /**
     * @brief Legge lo stato logico dei pin relativi alla porta specificata.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask dello stato logico dei pin (0: basso, 1: alto).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_gpio(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Imposta lo stato logico dei pin relativo alla porta specificata. La scrittura di
     * questo registro modifica il registro Output Latch (OLAT).
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @param[in] value                 Bitmask dello stato logico dei pin (0: basso, 1: alto).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_set_gpio(
        mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value);

    /**
     * @brief Legge la direzione dei pin relativi alla porta specificata.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask della direzione dei pin (0: output, 1: input).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_direction(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Imposta la direzione dei pin relativi alla porta specificata.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @param[in] value                 Bitmask della direzione dei pin (0: output, 1: input).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_set_direction(
        mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value);

    /**
     * @brief Legge la polarità dei pin relativi alla porta specificata.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask della polarità dei pin (0: valore uguale alla
     * logica del pin, 1: valore inverso alla logica del pin).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_polarity(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Imposta la polarità dei pin relativi alla porta specificata.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @param[in] value                 Bitmask della polarità dei pin (0: output, 1: input).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_set_polarity(
        mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value);

    /**
     * @brief Restituisce lo stato dei pin aventi un pull-up attivo.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask dei pull-up dei pin (0: abilitato, 1: altrimenti).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_pullup(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Imposta i pull-up interni dei pin selezionati nella bitmask.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @param[in] value                 Bitmask dei pull-up dei pin (0: abilitato, 1: altrimenti).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_set_pullup(
        mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value);

    /**
     * @brief Abilita gli interrupt dei pin selezionati nella bitmask.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @param[in] mode                  Modalità di attivazione dell'interrupt (0: entrambi i
     * fronti, 1: fronte di discesa, 2: fronte di salita).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_attach_int(
        mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t mode);

    /**
     * @brief Disabilita gli interrupt dei pin selezionati nella bitmask.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] port                  Valore della porta (0: porta A, 1: porta B).
     * @param[in] gpio                  Bitmask dei pin (0: ignorato, 1: altrimenti).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_detach_int(mcp23017_t *device, const uint8_t port, const uint8_t gpio);

    /**
     * @brief Restituisce lo stato logico dei pin al momento dell'interrupt. Non tiene conto
     * della modalità di trigger dell'interrupt. Resetta inoltre lo stato di interrupt.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask dello stato logico dei pin (0: basso, 1: alto).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_int_state(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Restituisce lo stato logico dei pin che hanno generato un interrupt. Non tiene conto
     * della modalità di trigger dell'interrupt.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] value                Bitmask dei pin (0: no interrupt, 1: altrimenti).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_int_source(mcp23017_t *device, const uint8_t port, uint8_t *value);

    /**
     * @brief Restituisce i pin che hanno generato l'interrupt con il relativo stato logico tenendo
     * conto della modalità di trigger dell'interrupt. Resetta inoltre lo stato di interrupt.
     *
     * @param[in]  device               Dispositivo MCP23017.
     * @param[in]  port                 Valore della porta (0: porta A, 1: porta B).
     * @param[out] source               Bitmask dei pin (0: no interrupt, 1: altrimenti).
     * @param[out] state                Bitmask dello stato logico dei pin (0: basso, 1: alto).
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_get_int_event(
        mcp23017_t *device, const uint8_t port, uint8_t *source, uint8_t *state);

    /**
     * @brief Registra la callback chiamata al verificarsi di un interrupt hardware.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] callback              Funzione invocata dalla ISR.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     */
    mcp23017_err_t mcp23017_register_isr_callback(
        mcp23017_t *device, mcp23017_isr_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // MCP23017_CORE_H
