# SD Card Changer for ESP32

This project implements a **multi-slot SD card changer** interface using ESP32, allowing selection, power control, and detection of multiple SD cards via an I2C-controlled GPIO expander (MCP23017). It enables dynamic switching between SD cards through software without physical reconnection.

## Features

- Support for multiple SD card slots (up to 8)
- Software-controlled SD card selection via I2C GPIO expander (MCP23017)
- Detection and power control of SD cards
- Integration with ESP-IDF and ESP32 SDMMC host

## Hardware Requirements

- ESP32-S3 dev kit
- SD Changer v2.0

## Wiring Overview

| Signal     | PORT A   | PORT B |
|------------|----------|--------|
| SD CLK     | GPIO6    |GPIOxx     |
| SD CMD     | GPIO7    |GPIOxx     |
| SD D0      | GPIO5    |GPIOxx     |
| SD D1      | GPIO4    |GPIOxx     |
| SD D2      | GPIO15   |GPIOxx     |
| SD D3      | GPIO16   |GPIOxx     |
| I2C SDA    | GPIO1    |
| I2C SCL    | GPIO2    |



## Software Overview

### Main Data Structures

- `sdchngr_handle_t`: Tracks selected slot, detected/powered cards, and handles to MCP23017 and I2C.
- `sdchngr_port_config_t`: Contains GPIO pin definitions for SDMMC and I2C.

## Build Instructions

- Add this module and dependencies (mcp23017, i2c_bus) to your ESP-IDF project.
- Include the header in your application
