#ifndef __NVS_DISPLAY_CONFIG_H__
#define __NVS_DISPLAY_CONFIG_H__

#include "nvs_flash.h"

#define DEFAULT_DISPLAY_OFFSET 0
#define DEFAULT_DISPLAY_MAX_BYTE 20
#define DEMO_DISPLAY_OFFSET (DEFAULT_DISPLAY_OFFSET+DEFAULT_DISPLAY_MAX_BYTE)
#define DEMO_DISPLAY_MAX_BYTE 20
#define DISPLAY_LIST_OFFSET (DEMO_DISPLAY_OFFSET+DEMO_DISPLAY_MAX_BYTE)
#define DISPLAY_LIST_MAX_BYTE 1024
typedef struct {
    char default_display[DEFAULT_DISPLAY_MAX_BYTE];
    char demo_display[DEMO_DISPLAY_MAX_BYTE];
    char display_list[DISPLAY_LIST_MAX_BYTE];
} display_config_t;

void nvs_test(void);
esp_err_t nvs_read_display_config(display_config_t *config);
esp_err_t nvs_read_default_display(char *default_display);
esp_err_t nvs_read_demo_display(char *demo_display);
esp_err_t nvs_read_display_list(char *display_list);
esp_err_t nvs_write_display_config(display_config_t *config);
// esp_err_t nvs_write_default_display(char *default_display);
// esp_err_t nvs_write_demo_display(char *demo_display);
// esp_err_t nvs_write_display_list(char *display_list);
esp_err_t nvs_erase_display_config_list(void);

#endif