set(srcs "nvs_display_config.c" )
set(includes ".")
set(priv_requires "esp_mm" "esp_psram" "driver" "esp_timer" "nvs_flash")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES ${priv_requires}
                       INCLUDE_DIRS  ${includes}
                       REQUIRES driver)