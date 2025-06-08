| Supported Targets | ESP32-S3 |
| ----------------- | -------- |


## SD Changer code example

This code demonstrates a use of SD-Changer library on 8-slot ESP32S3 SD-Changer device. 
The device is expected to be connected via serial port to allow this example work.

The SD-Changer specific code looks as follows:

```c
esp_sdchngr_dev_t changer = SDCHNGR_DEFAULT();
ESP_ERROR_CHECK(esp_sdchngr_init(&changer));
ESP_ERROR_CHECK(esp_sdchngr_set_power(changer, SLOT, 1));
ESP_ERROR_CHECK(esp_sdchngr_set_selected(changer, SLOT, &slot_config));
...
// process your application
...
```

To run the example code:

1. set proper IDF environment - for instance, run the following scripts in your SD changer working directory:
```
<ESP-IDF-INSTALL-PATH>/install.sh
. <ESP-IDF-INSTALL-PATH>/export.sh>
```

2. navigate to the example directory, set ESP32S3 target and build the example code. Flash the binary into the device as the last step
```
cd example
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

The console output should be similar to the following:
