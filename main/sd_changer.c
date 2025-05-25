#include "sd_changer.h"

/**
 * PHY GPIO extender pinout
 * GPA0 CD1
 * GPA1 CD2
 * GPA2 CD3
 * GPA3 CD4
 * GPA4 NC
 * GPA5 NC
 * GPA6 NC
 * GPA7 NC
 * GPB0 PE1
 * GPB1 OE1
 * GPB2 PE2
 * GPB3 OE2
 * GPB4 PE3
 * GPB5 OE3
 * GPB6 PE4
 * GPB7 OE4
 *
 */

static const char *TAG = "sdchngr";

#define SDCHNGR_CHECK(a, str, ret)                                             \
    if (!(a))                                                                  \
    {                                                                          \
        ESP_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret);                                                          \
    }

esp_err_t sdchngr_init(sdchngr_handle_t handle)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    
    // Init I2C peripherals
    i2c_config_t conf_i2c = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SD_PORTA_SDA,
        .scl_io_num = SD_PORTA_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
    };
    handle->i2c = i2c_bus_create(I2C_NUM_0, &conf_i2c);
    // Init mcp23017 GPIO expander
    // [mcp0] [A2:1][A1:1][A0:0]
    // [mcp1] [A2:1][A1:0][A0:0]
    handle->mcps[0] = mcp23017_create(handle->i2c, MCP23017_I2C_ADDRESS_DEFAULT + 6);
    handle->mcps[1] = mcp23017_create(handle->i2c, MCP23017_I2C_ADDRESS_DEFAULT + 4);

    mcp23017_set_io_dir(handle->mcps[0], 0x00, MCP23017_GPIOA); // Set as input pins
    mcp23017_set_io_dir(handle->mcps[0], 0xFF, MCP23017_GPIOB); // Set as output pins
    mcp23017_set_io_dir(handle->mcps[1], 0x00, MCP23017_GPIOA);
    mcp23017_set_io_dir(handle->mcps[1], 0xFF, MCP23017_GPIOB);
    mcp23017_write_io(handle->mcps[0], 0b10101010, MCP23017_GPIOB); // Off state
    mcp23017_write_io(handle->mcps[1], 0b10101010, MCP23017_GPIOB);

    ESP_LOGI(TAG, "Created\n");
    return ESP_OK;
}

esp_err_t sdchngr_set_selected(sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *slot_config)
{
    SDCHNGR_CHECK(handle != NULL && slot_config != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot >= SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
    if (!sdchngr_is_detected(handle, slot))
        return ESP_ERR_NOT_FOUND;

    sdchngr_set_port(handle, slot, slot_config);
    sdchngr_set_mcp(handle, slot);
    uint8_t gpio = slot;
    if (gpio >= 4)
        gpio -= 4;

    uint8_t current_gpio = mcp23017_read_io(handle->curentMcp, MCP23017_GPIOB);
    current_gpio |= 0b10101010;              // Apply mask
    current_gpio |= (0 << ((gpio * 2) + 1)); // OE pins stride 2, offset 1
    mcp23017_write_io(handle->curentMcp, current_gpio, MCP23017_GPIOB);
    handle->selectedSlot = slot;

    ESP_LOGI(TAG, "Selected slot %d\n", slot);
    return ESP_OK;
}

esp_err_t sdchngr_set_power(sdchngr_handle_t handle, uint8_t slot, bool power)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot >= SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
    if (!sdchngr_is_detected(handle, slot))
        return ESP_ERR_NOT_FOUND;

    sdchngr_set_mcp(handle, slot);
    if (slot >= 4)
        slot -= 4;

    uint8_t current_gpio = mcp23017_read_io(handle->curentMcp, MCP23017_GPIOB);
    current_gpio |= (1 << (slot * 2)); // OE pins stride 2, offset 0
    mcp23017_write_io(handle->curentMcp, current_gpio, MCP23017_GPIOB);
    handle->poweredSlots |= slot;

    ESP_LOGI(TAG, "Power [%d] slot %d\n", power, slot);
    return ESP_OK;
}

uint8_t sdchngr_get_selected(sdchngr_handle_t handle)
{
    return handle->selectedSlot;
}

esp_err_t sdchngr_get_detected(sdchngr_handle_t handle, uint8_t *nDetected, uint8_t *slots)
{
    SDCHNGR_CHECK(handle != NULL && nDetected != NULL && slots != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    uint8_t porta = mcp23017_read_io(handle->mcps[0], MCP23017_GPIOA);
    uint8_t portb = mcp23017_read_io(handle->mcps[1], MCP23017_GPIOA);
    porta = porta & 0x0F; // Only bits 0–3
    portb = portb & 0x0F;
    *slots = (portb << 4) | porta;
    for (int i = 0; i < 8; ++i)
    {
        if (*slots & (1 << i))
        {
            (*nDetected)++;
        }
    }
    return ESP_OK;
}

esp_err_t sdchngr_get_powered(sdchngr_handle_t handle, uint8_t *nPowered, uint8_t *slots)
{
    SDCHNGR_CHECK(handle != NULL && nPowered != NULL && slots != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    slots = &handle->poweredSlots;
    for (int i = 0; i < 8; ++i)
    {
        if (*slots & (1 << i))
        {
            (*nPowered)++;
        }
    }
    return ESP_OK;
}

bool sdchngr_is_selected(sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    return slot == handle->selectedSlot;
}

bool sdchngr_is_powered(sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    return (handle->poweredSlots & (1 << slot)) == 1;
}

bool sdchngr_is_detected(sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    uint8_t nDetected = 0;
    uint8_t detected = 0;
    sdchngr_get_detected(handle, &nDetected, &detected);

    return (detected & (1 << slot)) == 1;
}

esp_err_t sdchngr_set_port(sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *config)
{
    SDCHNGR_CHECK(handle != NULL && config != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    if (slot > SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
    // TODO:
    if (slot < 4)
        *config = handle->portConfigs[0];
    else
        *config = handle->portConfigs[1];

    return ESP_OK;
}

esp_err_t sdchngr_set_mcp(sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot > 8)
        return ESP_ERR_INVALID_ARG;
    if (slot <= 4)
        handle->curentMcp = handle->mcps[0];
    else
        handle->curentMcp = handle->mcps[1];
    return ESP_OK;
}
