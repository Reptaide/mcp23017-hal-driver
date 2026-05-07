// SPDX-License-Identifier: MIT

#ifndef MCP23017_PLATFORM_H
#define MCP23017_PLATFORM_H

#include "driver/i2c_types.h"
#include "mcp23017_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Inizializza l'interfaccia hardware del dispositivo MCP23017.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] bus_handle            Handle del bus I2C.
     * @param[in] i2c_address           Indirizzo I2C del dispositivo.
     * @param[in] i2c_scl_speed         Velocità di clock del dispositivo sul bus I2C.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     * @retval MCP23017_ERR_FAIL        Errore nella configurazione dell'ISR o del pin.
     */
    mcp23017_err_t mcp23017_init_hal(mcp23017_t *device,
        i2c_master_bus_handle_t bus_handle,
        const uint8_t i2c_address,
        const uint32_t i2c_scl_speed,
        const int32_t i2c_timeout);

    /**
     * @brief Configura i pin di interrupt per le porta A, B e registra la ISR.
     *
     * @param[in] device                Dispositivo MCP23017.
     * @param[in] int_pin_a             Valore del pin per la porta A.
     * @param[in] int_pin_b             Valore del pin per la porta B.
     * @retval MCP23017_ERR_OK          Successo.
     * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
     * @retval MCP23017_ERR_FAIL        Errore nella configurazione dell'ISR o del pin.
     */
    mcp23017_err_t mcp23017_hal_setup_int(mcp23017_t *device, int8_t int_pin_a, int8_t int_pin_b);

#ifdef __cplusplus
}
#endif

#endif // MCP23017_PLATFORM_H