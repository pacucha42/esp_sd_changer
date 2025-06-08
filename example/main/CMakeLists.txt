idf_component_register(SRCS "../../src/sd_changer.c" "sd_card_example_main.c"
                    INCLUDE_DIRS "." "../../include")


if(NOT CONFIG_SOC_SDMMC_HOST_SUPPORTED)
    fail_at_build_time(sdmmc ""
                             "Only ESP32 and ESP32-S3 targets are supported."
                             "Please refer README.md for more details")
endif()
