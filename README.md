# SD Card Changer for ESP32-S3

This project implements a **multi-slot SD card changer** interface using the ESP32-S3 dev kit, allowing dynamic software-based switching between up to 8 SD cards without physically reconnecting them. It utilizes an I2C-controlled GPIO expander (MCP23017) for slot selection, power control, and card detection and SN74CB3Q3245 for physical bus switching.

---

## Features

- Support for up to **8 SD card slots**
- Software-controlled slot selection
- **Power control** and **presence detection** of SD cards
- Compatible with **ESP-IDF**
- Works with **SDMMC host interface**
- Based on **SN74CB3Q3245** bus switch
- Data bandwith up to 500 MHz

---

## Hardware Requirements

- ESP32-S3 development kit
- SD Changer v2.0 board
- Other boards can be used wih wiring below

---

## Wiring Overview

| Signal     | PORT A                 | PORT B                 |
|------------|------------------------|------------------------|
| SD CLK     | GPIO6                  | GPIO42                |
| SD CMD     | GPIO7                  | GPIO41                |
| SD D0      | GPIO5                  | GPIO2                 |
| SD D1      | GPIO4                  | GPIO1                 |
| SD D2      | GPIO16                 | GPIO39                |
| SD D3      | GPIO15                 | GPIO40                |
| I2C SDA    | GPIO13                 | (shared)              |
| I2C SCL    | GPIO12                 | (shared)              |

---

## Hardware Overview

Custom PCB is used for this project which source can be found in project structure.
The design is intended to use ESP32-S3-DevKit.

Supply voltage of the board is 3.3V and can be provided by dev kit or externaly via pin headers. Shorting jumper on J5 pin header is used to select external/internal(dev kit) power supply. Pin header J6 is used for providing external voltage. Red LED is used to indicate that board is powered.
**IMPORTANT** Board does not provide any reverse voltage nor overcurrent protection circuits, use external power with caution!

SD Cards power connection is controlled via high-side switch using P-MOSFET. Red LED indicates that slot is beeing powered.

All of SD Slot communication lines have dedicated 10k pull up resistors. Slots are connected to the ESP through 8-bit bus switches SN74CB3Q3245. Board is divided into two halfs, each half is equiped with 4 of these switches and card slots. 

To control power, presense and bus switching logic, mcp23017 I2C GPIO expanders are used. GPIOA pins are used as inputs to detect if cards are present (internal pullups are in use). GPIOB pins are used to control power and bus selection logic. 

Maximum bandwidth of 500 MHz could theoretically be achieved although signal interference may not allow susch high speeds. 

---

## Software Overview

### Data Structures

- `esp_sdchngr_handle_t`: Handle for the SD changer device. Tracks:
  - Currently selected slot
  - Detected cards (bitmask)
  - Powered slots (bitmask)
  - Configurations for 2 SD ports
  - MCP23017 handles
  - Active MCP expander
  - I2C bus handle

### Initialization Macros

```c
#define SD_PORTA_INIT {
.clk = GPIO_NUM_6,
.cmd = GPIO_NUM_7,
.d0 = GPIO_NUM_5,
.d1 = GPIO_NUM_4,
.d2 = GPIO_NUM_16,
.d3 = GPIO_NUM_15,
.cd = SDMMC_SLOT_NO_CD,
.wp = SDMMC_SLOT_NO_WP,
.width = 4,
.flags = 0,
}

#define SD_PORTB_INIT {
.clk = GPIO_NUM_42,
.cmd = GPIO_NUM_41,
.d0 = GPIO_NUM_2,
.d1 = GPIO_NUM_1,
.d2 = GPIO_NUM_39,
.d3 = GPIO_NUM_40,
.cd = SDMMC_SLOT_NO_CD,
.wp = SDMMC_SLOT_NO_WP,
.width = 4,
.flags = 0,
}

#define SDCHNGR_DEFAULT() {
.selectedSlot = 0,
.detectedCards = 0,
.poweredSlots = 0,
.portConfigs = {SD_PORTA_INIT, SD_PORTB_INIT},
.mcps = {NULL, NULL},
.curentMcp = NULL,
.i2c = NULL,
}
```

### Build instructions
1. Clone or copy this module into your ESP-IDF project.
2. Add components: espressif/mcp23017
3. Include the header in your application:
```c
#include "sd_changer.h"
```
4. Build the project (see Example for more details)

## API Overview
### Initialization
```c
esp_err_t esp_sdchngr_init(esp_sdchngr_handle_t handle);
```

### Card control
- Select SD card slot
```c
esp_err_t esp_sdchngr_set_selected(esp_sdchngr_handle_t handle, uint8_t slot, sdmmc_slot_config_t *slot_config);
```
Note: card is selected only if is card is physicaly detected in slot. `sdmmc_slot_config_t` is output of this function, the config should be used to access the card in selected slot.

- Power SD card on/off:
```c
esp_err_t esp_sdchngr_set_power(esp_sdchngr_handle_t handle, uint8_t slot, uint8_t power);
```
Note: card is powered only if is card is physicaly detected in slot

### State Queries
- Get selected slot:
```c
uint8_t esp_sdchngr_get_selected(esp_sdchngr_handle_t handle);
```
Returns number of slot that is selected.

- Get detected cards:
```c
esp_err_t esp_sdchngr_get_detected(esp_sdchngr_handle_t handle, uint8_t *nDetected, uint8_t *slots);
```
Gets number of detected cards and bitwise representation of detected cards.

- Get powered cards:
```c
esp_err_t esp_sdchngr_get_powered(esp_sdchngr_handle_t handle, uint8_t *nPowered, uint8_t *slots);
```
Gets number of powered cards and bitwise representation of powered cards.
 
- Check if a slot is selected / powered / detected:
```c
bool esp_sdchngr_is_selected(esp_sdchngr_handle_t handle, uint8_t slot);
bool esp_sdchngr_is_powered(esp_sdchngr_handle_t handle, uint8_t slot);
bool esp_sdchngr_is_detected(esp_sdchngr_handle_t handle, uint8_t slot);
```
## Additional notes
- Each SD slot must be individually powered and detected before being selected.
- Physically two slots can be active at the same time but SW implementation uses only one. When selecting from slot at port A [0-3] to slot on port B [4-7], the bus switch is not deactivated on port A. API returns `sdmmc_slot_config_t` with pins to card on port B. Theoretically 2 slots can be active and be used by 2 SDMMC controllers at the same time.
- SD card can be switched without the need for unmounting them first but it is advised to do so.
- SD slots are physically on board numbered [1-8]. The SW uses numbering [0-7].
- the Example code can be used as a testing application