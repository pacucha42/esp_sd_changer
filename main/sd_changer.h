#pragma once

#include <stdint.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
// GPIO Expander
#include "mcp23017.h"
#include "i2c_bus.h"


#define SD_SLOT_COUNT 8
#define SD_PORT_COUNT 2

#define SD_I2C_SCL GPIO_NUM_12
#define SD_I2C_SDA GPIO_NUM_13

#define SD_PORTA_INIT {     \
    .clk = GPIO_NUM_6,    \
    .cmd = GPIO_NUM_7,    \
    .d0 = GPIO_NUM_5,      \
    .d1 = GPIO_NUM_4,      \
    .d2 = GPIO_NUM_16,      \
    .d3 = GPIO_NUM_15,      \
    .cd = SDMMC_SLOT_NO_CD, \
    .wp = SDMMC_SLOT_NO_WP, \
    .width = 4,             \
    .flags = 0,             \
}

#define SD_PORTB_INIT {     \
    .clk = GPIO_NUM_42,    \
    .cmd = GPIO_NUM_41,    \
    .d0 = GPIO_NUM_2,      \
    .d1 = GPIO_NUM_1,      \
    .d2 = GPIO_NUM_39,      \
    .d3 = GPIO_NUM_40,      \
    .cd = SDMMC_SLOT_NO_CD, \
    .wp = SDMMC_SLOT_NO_WP, \
    .width = 4,             \
    .flags = 0,             \
}
#define SDCHNGR_DEFAULT() { \
    .selectedSlot = 0, \
    .detectedCards = 0, \
    .poweredSlots = 0, \
    .portConfigs = {SD_PORTA_INIT, SD_PORTB_INIT}, \
    .mcps = {NULL, NULL}, \
    .curentMcp = NULL, \
    .i2c = NULL, \
}

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Struct containig GPIO pins definitions for SD changer port
    typedef struct sdchngr_port_config_s
    {
        /// @brief sdmmc pins
        gpio_num_t clk, cmd, d0, d1, d2, d3;
        /// @brief i2c pins
        gpio_num_t SDA, SCL;
    } sdchngr_port_config_t;

    /// @brief SD Changer handle variables struct
    typedef struct sdchngr_dev_s
    {
        uint8_t selectedSlot;
        uint8_t detectedCards;
        uint8_t poweredSlots;
        sdmmc_slot_config_t portConfigs[2];
        mcp23017_handle_t mcps[2];
        mcp23017_handle_t* curentMcp;
        i2c_bus_handle_t i2c;
    } sdchngr_dev_t;

    typedef sdchngr_dev_t* sdchngr_handle_t;

    /**
     * @brief Initialize SD Changer handle
     * Initializes sdmmc_host and i2c handle
     * @return
     *      - ESP_OK on success
     *
     */
    esp_err_t sdchngr_init(sdchngr_handle_t handle);

    /**
     * @brief Select SD card to be connected to the sdmmc host
     *
     * SD must be detected by handle to be selected
     * @param handle [IN] sdchanger config struct
     * @param slot [IN] SD card slot [1-8], 0 for none selected
     * @param slot_config [OUT] sdmmc slot config
     * @return
     *      - ESP_OK on success
     *      - ESP_ERR_NOT_FOUND if SD card is not detected
     *      - ESP_ERR_INVALID_ARG if slot is not supported
     */
    esp_err_t sdchngr_set_selected(sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *slot_config);

    /**
     * @brief Turn SD card on/off
     * SD must be detected by handle to be powered
     *
     * @param handle resulting configuration
     * @param slot SD card slot [0-7]
     * @param power false-OFF true-ON
     * @return
     *      - ESP_OK on success
     *      - ESP_ERR_NOT_FOUND if SD card is not detected
     */
    esp_err_t sdchngr_set_power(sdchngr_handle_t handle, uint8_t slot, bool power);

    /**
     * @brief Get number of selected slot
     *
     * @param handle configuration
     * @return [1-8] selected slot, 0 for none
     */
    uint8_t sdchngr_get_selected(sdchngr_handle_t handle);

    /**
     * @brief Get bitmask of detected cards
     *
     * @param handle configuration
     * @param nDetected number of detected cards
     * @param slots bitmask of slots with detected cards
     * @return
     *      - ESP_OK on success
     */
    esp_err_t sdchngr_get_detected(sdchngr_handle_t handle, uint8_t *nDetected, uint8_t *slots);

    /**
     * @brief Get bitmask of powered cards
     *
     * @param handle configuration
     * @param nPowered number of powered cards
     * @param slots bitmask of slots with powered cards
     * @return
     *      - ESP_OK on success
     */
    esp_err_t sdchngr_get_powered(sdchngr_handle_t handle, uint8_t *nPowered, uint8_t *slots);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if slot is selected
     */
    bool sdchngr_is_selected(sdchngr_handle_t handle, uint8_t slot);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if card is powered
     */
    bool sdchngr_is_powered(sdchngr_handle_t handle, uint8_t slot);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if card is detected
     */
    bool sdchngr_is_detected(sdchngr_handle_t handle, uint8_t slot);

    esp_err_t sdchngr_set_port(sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *slot_config);
    esp_err_t sdchngr_set_mcp(sdchngr_handle_t handle, uint8_t slot);

#ifdef __cplusplus
}
#endif