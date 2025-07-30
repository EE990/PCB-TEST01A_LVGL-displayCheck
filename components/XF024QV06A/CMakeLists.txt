set(srcs "esp_lcd_panel_XF024QV06A.c" )
set(includes "." "${IDF_PATH}/components/esp_lcd/include" "${IDF_PATH}/components/esp_lcd/interface")
set(priv_requires "esp_mm" "esp_psram" "driver" "esp_timer" "esp_lcd")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES ${priv_requires}
                       INCLUDE_DIRS  ${includes}
                       REQUIRES driver)