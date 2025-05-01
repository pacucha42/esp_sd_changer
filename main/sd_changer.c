#include "sd_changer.h"

esp_err_t sdchngr_init_default(sdchngr_handle_t *handle)
{
    // Init SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    handle->sdmmc = host;
    sdmmc_slot_config_t port = SD_PORTA_INIT;
    handle->portConfigs[0] = port;
    handle->portConfigs[1] = port;

    // Init I2C peripherals
    i2c_config_t conf_i2c = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SD_PORTA_SDA,
        .scl_io_num = SD_PORTA_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
    };
    handle->i2c = i2c_bus_create(I2C_NUM_0, &conf_i2c);
    handle->mcp = mcp23017_create(handle->i2c, MCP23017_I2C_ADDRESS_DEFAULT);
    
    mcp23017_set_io_dir(handle->mcp, 0, MCP23017_PIN0);
    // TODO:
    


    return ESP_OK;
}
