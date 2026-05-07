// SPDX-License-Identifier: MIT

#include "mcp23017_core.h"

/**
 * @brief Macro per interrompere l'esecuzione in caso una funzione restituisca un errore.
 */
#define MCP23017_ERROR_CHECK(func)                                                                 \
    do                                                                                             \
    {                                                                                              \
        mcp23017_err_t __err = (func);                                                             \
        if (__err != MCP23017_ERR_OK)                                                              \
            return __err;                                                                          \
    } while (0)

/**
 * @brief Macro per validare i parametri in input delle funzioni.
 */
#define MCP23017_INPUT_CHECK(device, port)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!device || !device->bank || port > 1)                                                  \
            return MCP23017_ERR_INVALID_ARG;                                                       \
    } while (0)

/**
 * @brief Definizione dei registri per il bank 0 (default) e per il bank 1.
 */
static const mcp23017_bank_t bank_0 = {.reg_iodir_a = 0x00,
    .reg_iodir_b = 0x01,
    .reg_ipol_a = 0x02,
    .reg_ipol_b = 0x03,
    .reg_gpinten_a = 0x04,
    .reg_gpinten_b = 0x05,
    .reg_defval_a = 0x06,
    .reg_defval_b = 0x07,
    .reg_intcon_a = 0x08,
    .reg_intcon_b = 0x09,
    .reg_iocon_a = 0x0A, // Il registro IOCON ha due indirizzi, ma sono equivalenti
    .reg_iocon_b = 0x0B, // Il registro IOCON ha due indirizzi, ma sono equivalenti
    .reg_gppu_a = 0x0C,
    .reg_gppu_b = 0x0D,
    .reg_intf_a = 0x0E,
    .reg_intf_b = 0x0F,
    .reg_intcap_a = 0x10,
    .reg_intcap_b = 0x11,
    .reg_gpio_a = 0x12,
    .reg_gpio_b = 0x13,
    .reg_olat_a = 0x14,
    .reg_olat_b = 0x15};

static const mcp23017_bank_t bank_1 = {.reg_iodir_a = 0x00,
    .reg_iodir_b = 0x10,
    .reg_ipol_a = 0x01,
    .reg_ipol_b = 0x11,
    .reg_gpinten_a = 0x02,
    .reg_gpinten_b = 0x12,
    .reg_defval_a = 0x03,
    .reg_defval_b = 0x13,
    .reg_intcon_a = 0x04,
    .reg_intcon_b = 0x14,
    .reg_iocon_a = 0x05, // Il registro IOCON ha due indirizzi, ma sono equivalenti
    .reg_iocon_b = 0x15, // Il registro IOCON ha due indirizzi, ma sono equivalenti
    .reg_gppu_a = 0x06,
    .reg_gppu_b = 0x16,
    .reg_intf_a = 0x07,
    .reg_intf_b = 0x17,
    .reg_intcap_a = 0x08,
    .reg_intcap_b = 0x18,
    .reg_gpio_a = 0x09,
    .reg_gpio_b = 0x19,
    .reg_olat_a = 0x0A,
    .reg_olat_b = 0x1A};

/**
 * @brief Definizione delle maschere
 */
static const uint8_t MASK_REG_IOCON_BANK = 0x80;
static const uint8_t MASK_REG_IOCON_MIRROR = 0x40;
static const uint8_t MASK_REG_IOCON_SEQOP = 0x20;
static const uint8_t MASK_REG_IOCON_DISSLW = 0x10;
static const uint8_t MASK_REG_IOCON_HAEN = 0x08;
static const uint8_t MASK_REG_IOCON_ODR = 0x04;
static const uint8_t MASK_REG_IOCON_INTPOL = 0x02;

/**
 * @brief Legge un byte (8 bit) da un registro del dispositivo, tramite il protocollo I2C.
 *
 * @param[in]  device               Dispositivo MCP23017.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[out] data                 Dati da leggere.
 * @retval MCP23017_ERR_OK          Successo.
 * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
 */
static mcp23017_err_t read_register_8(mcp23017_t *device, const uint8_t reg, uint8_t *data)
{
    // Verifica i parametri
    if (!device || !data)
        return MCP23017_ERR_INVALID_ARG;

    MCP23017_ERROR_CHECK(device->platform->i2c_read(device, reg, data));

    return MCP23017_ERR_OK;
}

/**
 * @brief Scive un byte (8 bit) in un registro del dispositivo, tramite il protocollo I2C.
 *
 * @param[in] device                Dispositivo MCP23017.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] data                  Dati da scrivere.
 * @retval MCP23017_ERR_OK          Successo.
 * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
 */
static mcp23017_err_t write_register_8(mcp23017_t *device, const uint8_t reg, const uint8_t data)
{
    // Verifica il parametro
    if (!device)
        return MCP23017_ERR_INVALID_ARG;

    MCP23017_ERROR_CHECK(device->platform->i2c_write(device, reg, data));

    return MCP23017_ERR_OK;
}

/**
 * @brief Legge, modifica e scrive un byte (8 bit) dello stato del registro specificato, tramite il
 * protocollo I2C.
 *
 * @param[in] device                Dispositivo MCP23017.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] bitmask               Bitmask dei bit da modificare.
 * @param[in] value                 Bitmask dello stato logico dei pin (0: basso, 1: alto).
 * @retval MCP23017_ERR_OK          Successo.
 * @retval MCP23017_ERR_INVALID_ARG Parametri non validi.
 */
static mcp23017_err_t rmw_register_8(
    mcp23017_t *device, const uint8_t reg, const uint8_t bitmask, const uint8_t value)
{
    // Verifica il parametro
    if (!device)
        return MCP23017_ERR_INVALID_ARG;

    // Legge lo stato attuale del registro
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg, &reg_val));

    // Imposta il valore dei pin specificati:
    // (config & ~bitmask) azzera i valori dei pin specificati
    // (value & bitmask) mantiene i soli valori dei pin specificati
    // L'OR modifica lo stato precedente con la nuova configurazione
    reg_val = (reg_val & ~bitmask) | (value & bitmask);

    // Scrive il nuovo stato nel registro
    MCP23017_ERROR_CHECK(write_register_8(device, reg, reg_val));

    return MCP23017_ERR_OK;
}

static inline uint8_t select_reg(const uint8_t reg_a, const uint8_t reg_b, const uint8_t port)
{
    return port == 0 ? reg_a : reg_b;
}

mcp23017_err_t mcp23017_init_core(mcp23017_t *device, const uint8_t config)
{
    // Verifica i parametri
    if (!device || !device->platform)
        return MCP23017_ERR_INVALID_ARG;

    // All'avvio il dispositivo è impostato su BANK = 0
    device->bank = &bank_0;

    // Applica la configurazione al dispositivo
    MCP23017_ERROR_CHECK(mcp23017_set_config(device, 0xFF, config));

    // Inizializza lo stato precedente dei pin per le porte A/B
    uint8_t gpio;
    mcp23017_get_gpio(device, MCP23017_PORT_A, &gpio);
    device->last_pin_state[MCP23017_PORT_A] = gpio;

    mcp23017_get_gpio(device, MCP23017_PORT_B, &gpio);
    device->last_pin_state[MCP23017_PORT_B] = gpio;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_config(mcp23017_t *device, uint8_t *value)
{
    // Verifica i parametri
    if (!device || !device->bank || !value)
        return MCP23017_ERR_INVALID_ARG;

    // Questo registro è agnostico rispetto alla porta selezionata.
    const uint8_t reg_iocon = device->bank->reg_iocon_a;

    // Legge lo stato del registro IOCON
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_iocon, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_set_config(mcp23017_t *device, const uint8_t bitmask, const uint8_t value)
{
    // Verifica i parametri
    if (!device || !device->bank)
        return MCP23017_ERR_INVALID_ARG;

    // Questo registro è agnostico rispetto alla porta selezionata.
    const uint8_t reg_iocon = device->bank->reg_iocon_a;

    // Legge lo stato del registro IOCON
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_iocon, &reg_val));

    reg_val = (reg_val & ~bitmask) | (value & bitmask);

    // Prima scrittura con l'indirizzo attuale (prima del cambio di BANK)
    MCP23017_ERROR_CHECK(write_register_8(device, reg_iocon, reg_val));

    // Seconda scrittura con l'indirizzo aggiornato (dopo il cambio di BANK nel chip)
    MCP23017_ERROR_CHECK(write_register_8(device, device->bank->reg_iocon_a, reg_val));

    // Imposta la mappa dei registri in base al valore di BANK
    device->bank = (reg_val & MASK_REG_IOCON_BANK) ? &bank_1 : &bank_0;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_gpio(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gpio = select_reg(device->bank->reg_gpio_a, device->bank->reg_gpio_b, port);

    // Legge lo stato del registro GPIO
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_gpio, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_set_gpio(
    mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gpio = select_reg(device->bank->reg_gpio_a, device->bank->reg_gpio_b, port);

    // Aggiorna lo stato del registro GPIO
    rmw_register_8(device, reg_gpio, gpio, value);

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_direction(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_iodir =
        select_reg(device->bank->reg_iodir_a, device->bank->reg_iodir_b, port);

    // Legge lo stato del registro IODIR
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_iodir, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_set_direction(
    mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_iodir =
        select_reg(device->bank->reg_iodir_a, device->bank->reg_iodir_b, port);

    // Aggiorna lo stato del registro IODIR
    rmw_register_8(device, reg_iodir, gpio, value);

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_polarity(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_ipol = select_reg(device->bank->reg_ipol_a, device->bank->reg_ipol_b, port);

    // Legge lo stato del registro IPOL
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_ipol, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_set_polarity(
    mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_ipol = select_reg(device->bank->reg_ipol_a, device->bank->reg_ipol_b, port);

    // Aggiorna lo stato del registro IPOL
    rmw_register_8(device, reg_ipol, gpio, value);

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_pullup(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gppu = select_reg(device->bank->reg_gppu_a, device->bank->reg_gppu_b, port);

    // Legge lo stato del registro GPPU
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_gppu, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_set_pullup(
    mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gppu = select_reg(device->bank->reg_gppu_a, device->bank->reg_gppu_b, port);

    // Aggiorna lo stato del registro GPPU
    rmw_register_8(device, reg_gppu, gpio, value);

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_attach_int(
    mcp23017_t *device, const uint8_t port, const uint8_t gpio, const uint8_t mode)
{
    // Verifica i parametri
    if (!device || !device->bank || port > 1 || mode > 2)
        return MCP23017_ERR_INVALID_ARG;

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gpinten =
        select_reg(device->bank->reg_gpinten_a, device->bank->reg_gpinten_b, port);
    const uint8_t reg_intcon =
        select_reg(device->bank->reg_intcon_a, device->bank->reg_intcon_b, port);

    // Disabilita la comparazione dei bit con DEFVAL impostando a zero INTCON
    MCP23017_ERROR_CHECK(rmw_register_8(device, reg_intcon, gpio, 0x00));

    // Abilita gli interrupt sui pin
    MCP23017_ERROR_CHECK(rmw_register_8(device, reg_gpinten, gpio, gpio));

    // Salva la modalità di attivazione degli eventi di interrupt
    device->int_mode[port] = mode;

    // Resetta eventuali interrupt attivi sulla porta
    uint8_t dummy = 0;
    mcp23017_get_int_state(device, port, &dummy);

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_detach_int(mcp23017_t *device, const uint8_t port, const uint8_t gpio)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_gpinten =
        select_reg(device->bank->reg_gpinten_a, device->bank->reg_gpinten_b, port);
    const uint8_t reg_intcon =
        select_reg(device->bank->reg_intcon_a, device->bank->reg_intcon_b, port);
    const uint8_t reg_defval =
        select_reg(device->bank->reg_defval_a, device->bank->reg_defval_b, port);

    // Disabilita gli interrupt sui pin
    MCP23017_ERROR_CHECK(rmw_register_8(device, reg_gpinten, gpio, 0x00));

    // Resetta lo stato del registro INTCON
    MCP23017_ERROR_CHECK(rmw_register_8(device, reg_intcon, gpio, 0x00));

    // Resetta lo stato del registro DEFVAL
    MCP23017_ERROR_CHECK(rmw_register_8(device, reg_defval, gpio, 0x00));

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_int_state(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_intcap =
        select_reg(device->bank->reg_intcap_a, device->bank->reg_intcap_b, port);

    // Legge lo stato del registro INTCAP
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_intcap, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_int_source(mcp23017_t *device, const uint8_t port, uint8_t *value)
{
    // Verifica i parametri
    MCP23017_INPUT_CHECK(device, port);

    // Seleziona il valore del registro in base alla porta
    const uint8_t reg_intf = select_reg(device->bank->reg_intf_a, device->bank->reg_intf_b, port);

    // Legge lo stato del registro INTF
    uint8_t reg_val = 0;
    MCP23017_ERROR_CHECK(read_register_8(device, reg_intf, &reg_val));

    *value = reg_val;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_get_int_event(
    mcp23017_t *device, const uint8_t port, uint8_t *source, uint8_t *state)
{
    if (!device || !source || !state || port > 1)
        return MCP23017_ERR_INVALID_ARG;

    // Lettura della sorgente di interrupt
    uint8_t intf = 0;
    MCP23017_ERROR_CHECK(mcp23017_get_int_source(device, port, &intf));

    // Ritorna nel caso non ci siano interrupt
    if (!intf)
    {
        *source = 0x00;
        return MCP23017_ERR_OK;
    }

    // Ottiene lo stato dei pin al momento dell'interrupt e resetta quest'ultimo
    uint8_t intcap = 0;
    MCP23017_ERROR_CHECK(mcp23017_get_int_state(device, port, &intcap));

    *state = intcap;

    // Definisce lo stato precedente e attuale dei pin
    uint8_t prev = device->last_pin_state[port];
    uint8_t curr = intcap;

    // Determina l'edge del trigger
    uint8_t rising = (~prev) & curr;
    uint8_t falling = prev & (~curr);

    uint8_t result = 0;

    switch (device->int_mode[port])
    {
        case MCP23017_INT_RISING:
            result = rising & intf;
            break;

        case MCP23017_INT_FALLING:
            result = falling & intf;
            break;

        case MCP23017_INT_BOTH:
            result = (rising | falling) & intf;
            break;

        default:
            result = 0x00;
            break;
    }

    // Aggiorna lo stato precedente dei pin con lo stato attuale
    device->last_pin_state[port] = curr;

    *source = result;

    return MCP23017_ERR_OK;
}

mcp23017_err_t mcp23017_register_isr_callback(mcp23017_t *device, mcp23017_isr_callback_t callback)
{
    // Verifica i parametri
    if (!device || !callback)
        return MCP23017_ERR_INVALID_ARG;

    device->isr_callback = callback;

    return MCP23017_ERR_OK;
}
