set(srcs "esp_lcd_panel_240320W-C001.c" )
set(includes "." "${IDF_PATH}/components/esp_lcd/include" "${IDF_PATH}/components/esp_lcd/interface")
set(priv_requires "esp_mm" "esp_psram")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES ${priv_requires}
                       INCLUDE_DIRS  ${includes}
                       REQUIRES driver)