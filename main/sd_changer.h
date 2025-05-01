#pragma once

#include <stdint.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
// GPIO Expander
#include "mcp23017.h"
#include "i2c_bus.h"


#define SD_SLOT_COUNT 2
#define SD_PORT_COUNT 1

#define SD_PORTA_CLK GPIO_NUM_6
#define SD_PORTA_CMD GPIO_NUM_7
#define SD_PORTA_D0 GPIO_NUM_5
#define SD_PORTA_D1 GPIO_NUM_4
#define SD_PORTA_D2 GPIO_NUM_15
#define SD_PORTA_D3 GPIO_NUM_16
#define SD_PORTA_SDA GPIO_NUM_1
#define SD_PORTA_SCL GPIO_NUM_2
#define SD_PORTA_BITMASK (1ULL << SD_PORTA_CLK) | (1ULL << SD_PORTA_CMD) |                           \
                             (1ULL << SD_PORTA_D0) | (1ULL << SD_PORTA_D1) | (1ULL << SD_PORTA_D2) | \
                             (1ULL << SD_PORTA_D3) | (1ULL << SD_PORTA_S1) | (1ULL << SD_PORTA_S2)
#define SD_PORTA_INIT {     \
    .clk = SD_PORTA_CLK,    \
    .cmd = SD_PORTA_CMD,    \
    .d0 = SD_PORTA_D0,      \
    .d1 = SD_PORTA_D1,      \
    .d2 = SD_PORTA_D2,      \
    .d3 = SD_PORTA_D3,      \
    .cd = SDMMC_SLOT_NO_CD, \
    .wp = SDMMC_SLOT_NO_WP, \
    .width = 4,             \
    .flags = 0,             \
}
// #define SD_PORTB_CLK -1
// #define SD_PORTB_CMD -1
// #define SD_PORTB_D1 -1
// #define SD_PORTB_D2 -1
// #define SD_PORTB_D3 -1
// #define SD_PORTB_D4 -1

/* #define SDMMC_SLOT_CONFIG_DEFAULT() {\
//     .clk = SD_PORTA_CLK, \
//     .cmd = SD_PORTA_CMD, \
//     .d0 = SD_PORTA_D0, \
//     .d1 = SD_PORTA_D1, \
//     .d2 = SD_PORTA_D2, \
//     .d3 = SD_PORTA_D3, \
// }*/


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
    typedef struct sdchngr_handle_s
    {
        uint8_t selectedSlot;
        uint8_t detectedCards;
        uint8_t poweredCards;
        sdmmc_slot_config_t portConfigs[2];
        sdmmc_host_t sdmmc;
        mcp23017_handle_t mcp;
        i2c_bus_handle_t i2c;
    } sdchngr_handle_t;

    /**
     * @brief Initialize SD Changer handle
     * Initializes sdmmc_host and i2c handle
     * @return
     *      - ESP_OK on success
     *
     */
    esp_err_t sdchngr_init_default(sdchngr_handle_t *handle);

    /**
     * @brief Select SD card to be connected to the sdmmc host
     *
     * SD must be detected by handle to be selected
     * @param handle resulting configuration
     * @param slot SD card slot [1-8], 0 for none selected
     * @return
     *      - ESP_OK on success
     *      - ESP_ERR_NOT_FOUND if SD card is not detected
     */
    esp_err_t sdchngr_set_selected(sdchngr_handle_t *handle, int slot);

    /**
     * @brief Turn SD card on/off
     * SD must be detected by handle to be powered
     *
     * @param handle resulting configuration
     * @param slot SD card slot [1-8]
     * @param power 0-OFF 1-ON
     * @return
     *      - ESP_OK on success
     *      - ESP_ERR_NOT_FOUND if SD card is not detected
     */
    esp_err_t sdchngr_set_power(sdchngr_handle_t *handle, int slot, int power);

    /**
     * @brief Get number of selected slot
     *
     * @param handle configuration
     * @return [1-8] selected slot, 0 for none
     */
    uint8_t sdchngr_get_selected(sdchngr_handle_t *handle);

    /**
     * @brief Get bitmask of detected cards
     *
     * @param handle configuration
     * @param nDetected number of detected cards
     * @param slots bitmask of slots with detected cards
     * @return
     *      - ESP_OK on success
     */
    esp_err_t sdchngr_get_detected(sdchngr_handle_t *handle, uint8_t *nDetected, uint8_t *slots);

    /**
     * @brief Get bitmask of powered cards
     *
     * @param handle configuration
     * @param nPowered number of powered cards
     * @param slots bitmask of slots with powered cards
     * @return
     *      - ESP_OK on success
     */
    esp_err_t sdchngr_get_powered(sdchngr_handle_t *handle, uint8_t *nPowered, uint8_t *slots);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if slot is selected
     */
    bool sdchngr_is_selected(sdchngr_handle_t *handle, int slot);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if card is powered
     */
    bool sdchngr_is_powered(sdchngr_handle_t *handle, int slot);

    /**
     * @brief
     *
     * @param handle configuration
     * @param slot sd card slot
     * @return true if card is detected
     */
    bool sdchngr_is_detected(sdchngr_handle_t *handle, int slot);

#ifdef __cplusplus
}
#endif