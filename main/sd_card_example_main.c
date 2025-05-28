/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_changer.h"
#include "driver/sdmmc_host.h"
#include "sd_changer.h"

#define EXAMPLE_MAX_CHAR_SIZE 64

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos)
    {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret = ESP_OK;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    sdchngr_dev_t device = SDCHNGR_DEFAULT();
    sdchngr_handle_t changer = (sdchngr_handle_t)&device;
    sdchngr_init(changer);
    uint8_t nDetected = 0;
    uint8_t detected = 0;
    sdchngr_get_detected(changer, &nDetected, &detected);
    ESP_LOGI(TAG, "Detected num %d mask %02x", nDetected, detected);

    for (size_t i = 0; i < 8; i++)
    {
        ESP_LOGI(TAG, "[SLOT %d]", i);
        ret = sdchngr_set_power(changer, i, true);
        ret = sdchngr_set_selected(changer, i, &slot_config);
        if (ret != ESP_OK)
        {
            if (ret == ESP_ERR_NOT_FOUND)
                ESP_LOGW(TAG, "[SLOT %d] Card not inserted!", i);

            continue;
        }

        ESP_LOGI(TAG, "Mounting filesystem");
        ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

        if (ret != ESP_OK)
        {
            if (ret == ESP_FAIL)
            {
                ESP_LOGE(TAG, "Failed to mount filesystem. "
                              "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                              "Make sure SD card lines have pull-up resistors in place.",
                         esp_err_to_name(ret));
            }
            continue;
        }
        ESP_LOGI(TAG, "Filesystem mounted");

        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);

        // Use POSIX and C standard library functions to work with files:

        // First create a file.
        const char *file_hello = MOUNT_POINT "/hello.txt";
        char data[EXAMPLE_MAX_CHAR_SIZE];
        snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
        ret = s_example_write_file(file_hello, data);
        if (ret != ESP_OK)
        {
            continue;
        }

        const char *file_foo = MOUNT_POINT "/foo.txt";
        // Check if destination file exists before renaming
        struct stat st;
        if (stat(file_foo, &st) == 0)
        {
            // Delete it if it exists
            unlink(file_foo);
        }

        // Rename original file
        ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
        if (rename(file_hello, file_foo) != 0)
        {
            ESP_LOGE(TAG, "Rename failed");
            continue;
        }

        ret = s_example_read_file(file_foo);
        if (ret != ESP_OK)
        {
            continue;
        }

        // Format FATFS
#ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
        ret = esp_vfs_fat_sdcard_format(mount_point, card);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
            return;
        }

        if (stat(file_foo, &st) == 0)
        {
            ESP_LOGI(TAG, "file still exists");
            return;
        }
        else
        {
            ESP_LOGI(TAG, "file doesnt exist, format done");
        }
#endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

        const char *file_nihao = MOUNT_POINT "/nihao.txt";
        memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
        snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
        ret = s_example_write_file(file_nihao, data);
        if (ret != ESP_OK)
        {
            continue;
        }

        // Open file for reading
        ret = s_example_read_file(file_nihao);
        if (ret != ESP_OK)
        {
            continue;;
        }

        // All done, unmount partition and disable SDMMC peripheral
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        ESP_LOGI(TAG, "Card unmounted");
    }
}
