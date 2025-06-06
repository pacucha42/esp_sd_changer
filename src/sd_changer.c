#include "sd_changer.h"

#define SD_MCP_PULLUPS MCP23017_PIN0 |     \
                           MCP23017_PIN1 | \
                           MCP23017_PIN2 | \
                           MCP23017_PIN3 | \
                           MCP23017_PIN4 | \
                           MCP23017_PIN5 | \
                           MCP23017_PIN6 | \
                           MCP23017_PIN7

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)         \
    ((byte) & 0x80 ? '1' : '0'),     \
        ((byte) & 0x40 ? '1' : '0'), \
        ((byte) & 0x20 ? '1' : '0'), \
        ((byte) & 0x10 ? '1' : '0'), \
        ((byte) & 0x08 ? '1' : '0'), \
        ((byte) & 0x04 ? '1' : '0'), \
        ((byte) & 0x02 ? '1' : '0'), \
        ((byte) & 0x01 ? '1' : '0')

#define bitset(byte, nbit) ((byte) |= (1 << (nbit)))
#define bitclear(byte, nbit) ((byte) &= ~(1 << (nbit)))
#define bitflip(byte, nbit) ((byte) ^= (1 << (nbit)))
#define bitcheck(byte, nbit) ((byte) & (1 << (nbit)))

static const char *TAG = "sdchngr";

#define SDCHNGR_CHECK(a, str, ret)                                             \
    if (!(a))                                                                  \
    {                                                                          \
        ESP_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret);                                                          \
    }

esp_err_t esp_sdchngr_init(esp_sdchngr_handle_t handle)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    ESP_LOGI(TAG, "Initializing sd changer\n");
    // Init I2C peripherals
    i2c_config_t conf_i2c = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SD_I2C_SDA,
        .scl_io_num = SD_I2C_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = I2C_CLK_SRC_DEFAULT,
        .master.clk_speed = 100000};
    handle->i2c = i2c_bus_create(I2C_NUM_0, &conf_i2c);

    uint32_t speed = i2c_bus_get_current_clk_speed(handle->i2c);
    ESP_LOGI(TAG, "I2C speed %ld", speed);
    // Init mcp23017 GPIO expander
    // [mcp0] [A2:1][A1:1][A0:0]
    // [mcp1] [A2:1][A1:0][A0:0]
    handle->mcps[0] = mcp23017_create(handle->i2c, MCP23017_I2C_ADDRESS_DEFAULT + 6);
    handle->mcps[1] = mcp23017_create(handle->i2c, MCP23017_I2C_ADDRESS_DEFAULT + 4);

    mcp23017_set_io_dir(handle->mcps[0], 0xFF, MCP23017_GPIOA); // Set as input pins
    mcp23017_set_io_dir(handle->mcps[0], 0x00, MCP23017_GPIOB); // Set as output pins
    mcp23017_set_io_dir(handle->mcps[1], 0xFF, MCP23017_GPIOA);
    mcp23017_set_io_dir(handle->mcps[1], 0x00, MCP23017_GPIOB);
    mcp23017_set_pullup(handle->mcps[0], SD_MCP_PULLUPS);
    mcp23017_set_pullup(handle->mcps[1], SD_MCP_PULLUPS);
    mcp23017_write_io(handle->mcps[0], 0xFF, MCP23017_GPIOB); // Off state
    mcp23017_write_io(handle->mcps[1], 0xFF, MCP23017_GPIOB);

    ESP_LOGI(TAG, "Initialized\n");
    return ESP_OK;
}

esp_err_t esp_sdchngr_set_selected(esp_sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *slot_config)
{
    SDCHNGR_CHECK(handle != NULL && slot_config != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot >= SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
    if (!esp_sdchngr_is_detected(handle, slot))
        return ESP_ERR_NOT_FOUND;

    esp_sdchngr_set_port(handle, slot, slot_config);    // Get correct slot config
    esp_sdchngr_set_mcp(handle, slot);                  // Set correct currentMcp
    uint8_t gpio = slot;
    if (gpio >= 4)
        gpio -= 4;
    // Outputs are on GPIOB
    uint8_t current_gpio = mcp23017_read_io(handle->curentMcp, MCP23017_GPIOB);
    current_gpio |= 0b10101010; // Apply mask - odd bits are bus select
    // Clear bit, stride 2, offset 1, 0 is select active
    bitclear(current_gpio, ((slot % 4) * 2 + 1));
    mcp23017_write_io(handle->curentMcp, current_gpio, MCP23017_GPIOB);
    handle->selectedSlot = slot;

    ESP_LOGD(TAG, "Selected %d", slot);
    return ESP_OK;
}

esp_err_t esp_sdchngr_set_power(esp_sdchngr_handle_t handle, uint8_t slot, uint8_t power)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot >= SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
    if (!esp_sdchngr_is_detected(handle, slot))
        return ESP_ERR_NOT_FOUND;

    esp_sdchngr_set_mcp(handle, slot);  // Set corrent currentMcp
    // Outputs are on GPIOB
    uint8_t current_gpio = mcp23017_read_io(handle->curentMcp, MCP23017_GPIOB);
    // Set/clear bit, stride 2, offset 0, 0 is power active
    power >= true ? bitclear(current_gpio, ((slot % 4) * 2)) : bitset(current_gpio, ((slot % 4) * 2));
    mcp23017_write_io(handle->curentMcp, current_gpio, MCP23017_GPIOB);
    power >=1 ? bitset(handle->poweredSlots, slot) : bitclear(handle->poweredSlots, slot);

    ESP_LOGD(TAG, "Power [%d] slot %d\n", power, slot);
    return ESP_OK;
}

uint8_t esp_sdchngr_get_selected(esp_sdchngr_handle_t handle)
{
    return handle->selectedSlot;
}

esp_err_t esp_sdchngr_get_detected(esp_sdchngr_handle_t handle, uint8_t *nDetected, uint8_t *slots)
{
    SDCHNGR_CHECK(handle != NULL && nDetected != NULL && slots != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    uint8_t porta = mcp23017_read_io(handle->mcps[0], MCP23017_GPIOA);
    uint8_t portb = mcp23017_read_io(handle->mcps[1], MCP23017_GPIOA);
    // porta = porta & 0x0F; // Only bits 0–3
    // portb = portb & 0x0F;
    *slots = 0;
    // Combine 4 bits from both ports so the result is
    // 0b[BBBBAAAA]
    *slots = (portb << 4) | porta;
    *slots = ~(*slots); // Negate so that 1 represents detected card
    for (int i = 0; i < 8; ++i)
    {
        if (*slots & (1 << i))
            (*nDetected)++;
    }
    ESP_LOGD(TAG, "Detected: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*slots));
    return ESP_OK;
}

esp_err_t esp_sdchngr_get_powered(esp_sdchngr_handle_t handle, uint8_t *nPowered, uint8_t *slots)
{
    SDCHNGR_CHECK(handle != NULL && nPowered != NULL && slots != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    slots = &handle->poweredSlots;
    for (int i = 0; i < 8; ++i)
    {
        if (*slots & (1 << i))
            (*nPowered)++;
    }
    return ESP_OK;
}

uint8_t esp_sdchngr_is_selected(esp_sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    return slot == handle->selectedSlot;
}

uint8_t esp_sdchngr_is_powered(esp_sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    ESP_LOGD(TAG, "Powered: " BYTE_TO_BINARY_PATTERN " slot %d", BYTE_TO_BINARY(handle->poweredSlots), slot);
    return bitcheck(handle->poweredSlots, slot);
}

uint8_t esp_sdchngr_is_detected(esp_sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    uint8_t nDetected = 0;
    uint8_t detected = 0;
    esp_sdchngr_get_detected(handle, &nDetected, &detected);
    ESP_LOGD(TAG, "Slot %d detected [%d]", slot, bitcheck(detected, slot) > 0);
    return bitcheck(detected, slot) > 0;
}

esp_err_t esp_sdchngr_set_port(esp_sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *config)
{
    SDCHNGR_CHECK(handle != NULL && config != NULL, "invalid arg", ESP_ERR_INVALID_ARG);

    if (slot > SD_SLOT_COUNT)
        return ESP_ERR_INVALID_ARG;
        
    if (slot < 4)
        *config = handle->portConfigs[0];
    else
        *config = handle->portConfigs[1];

    return ESP_OK;
}

esp_err_t esp_sdchngr_set_mcp(esp_sdchngr_handle_t handle, uint8_t slot)
{
    SDCHNGR_CHECK(handle != NULL, "invalid arg", ESP_ERR_INVALID_ARG);
    if (slot > 8)
        return ESP_ERR_INVALID_ARG;
    if (slot < 4)
        handle->curentMcp = handle->mcps[0];
    else
        handle->curentMcp = handle->mcps[1];

    return ESP_OK;
}
