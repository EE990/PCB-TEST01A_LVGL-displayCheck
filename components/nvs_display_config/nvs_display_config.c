#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_display_config.h"

static const char *TAG = "nvs_display_config";

#define MY_NVS_PARTITION_TYPE 0x40
#define MY_NVS_PARTITION_SUBTYPE 0x40

// Get the string name of type enum values used in this example
static const char* get_type_str(esp_partition_type_t type)
{
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "ESP_PARTITION_TYPE_APP";
        case ESP_PARTITION_TYPE_DATA:
            return "ESP_PARTITION_TYPE_DATA";
        default:
            return "UNKNOWN_PARTITION_TYPE"; // type not used in this example
    }
}

// Get the string name of subtype enum values used in this example
static const char* get_subtype_str(esp_partition_subtype_t subtype)
{
    switch(subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_NVS:
            return "ESP_PARTITION_SUBTYPE_DATA_NVS";
        case ESP_PARTITION_SUBTYPE_DATA_PHY:
            return "ESP_PARTITION_SUBTYPE_DATA_PHY";
        case ESP_PARTITION_SUBTYPE_APP_FACTORY:
            return "ESP_PARTITION_SUBTYPE_APP_FACTORY";
        case ESP_PARTITION_SUBTYPE_DATA_FAT:
            return "ESP_PARTITION_SUBTYPE_DATA_FAT";
        default:
            return "UNKNOWN_PARTITION_SUBTYPE"; // subtype not used in this example
    }
}

// Find the partition using given parameters
static void find_partition(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* name)
{
    ESP_LOGI(TAG, "Find partition with type %s, subtype %s, label %s...", get_type_str(type), get_subtype_str(subtype),
                    name == NULL ? "NULL (unspecified)" : name);

    const esp_partition_t * part  = esp_partition_find_first(type, subtype, name);

    if (part != NULL) {
        ESP_LOGI(TAG, "\tfound partition '%s' at offset 0x%" PRIx32 " with size 0x%" PRIx32, part->label, part->address, part->size);
    } else {
        ESP_LOGE(TAG, "\tpartition not found!");
    }
}

void nvs_test() {
    find_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    find_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, NULL);
    find_partition(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

    find_partition((esp_partition_type_t)MY_NVS_PARTITION_TYPE, (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE, "nvs_display_cf");
}

/**
 * @brief 從 NVS 分區讀取顯示器配置
 * @param config 指向 display_config_t 結構的指標，用於存儲讀取的配置
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_read_display_config(display_config_t *config) {
    esp_err_t err;

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    err = esp_partition_read(part, 0, config, sizeof(display_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

/**
 * @brief 從 NVS 分區讀取預設顯示器
 * @param default_display 指向要存儲預設顯示器的指標
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_read_default_display(char *default_display) {
    esp_err_t err;

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    err = esp_partition_read(part, DEFAULT_DISPLAY_OFFSET, default_display, DEFAULT_DISPLAY_MAX_BYTE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

/**
 * @brief 從 NVS 分區讀取示範顯示器
 * @param demo_display 指向要存儲示範顯示器的指標
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_read_demo_display(char *demo_display) {
    esp_err_t err;

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    err = esp_partition_read(part, DEMO_DISPLAY_OFFSET, demo_display, DEMO_DISPLAY_MAX_BYTE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

/**
 * @brief 從 NVS 分區讀取顯示器清單
 * @param display_list 指向要存儲顯示器清單的指標
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_read_display_list(char *display_list) {
    esp_err_t err;

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    err = esp_partition_read(part, DISPLAY_LIST_OFFSET, display_list, DISPLAY_LIST_MAX_BYTE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

/**
 * @brief 將顯示器配置寫入 NVS 分區
 * @param config 指向要寫入的 display_config_t 結構的指標
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_write_display_config(display_config_t *config) {
    esp_err_t err;

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    // ESP_LOGI(TAG, "label: %s", part->label);
    // ESP_LOGI(TAG, "address: %ld", part->address);
    // ESP_LOGI(TAG, "size: %ld", part->size);
    // ESP_LOGI(TAG, "encrypted: %d", part->encrypted);
    // ESP_LOGI(TAG, "readonly: %d", part->readonly);

    err = esp_partition_erase_range(part, 0, part->size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_partition_write(part, 0, config, sizeof(display_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

// /**
//  * @brief 將預設顯示器寫入 NVS 分區
//  * @param default_display 指向要寫入的預設顯示器的指標
//  * @return esp_err_t 操作結果
//  */
//  esp_err_t nvs_write_default_display(char *default_display) {
//     esp_err_t err;

//     const esp_partition_t* part = esp_partition_find_first(
//         (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
//         (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
//         "nvs_display_cf"
//     );

//     if (!part) {
//         ESP_LOGE(TAG, "Partition not found!");
//         return ESP_ERR_NOT_FOUND;
//     }

//     err = esp_partition_erase_range(part, DEFAULT_DISPLAY_OFFSET, DEFAULT_DISPLAY_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     err = esp_partition_write(part, DEFAULT_DISPLAY_OFFSET, default_display, DEFAULT_DISPLAY_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     return err;
// }

// /**
//  * @brief 將示範顯示器寫入 NVS 分區
//  * @param demo_display 指向要寫入的示範顯示器的指標
//  * @return esp_err_t 操作結果
//  */
// esp_err_t nvs_write_demo_display(char *demo_display) {
//     esp_err_t err;

//     const esp_partition_t* part = esp_partition_find_first(
//         (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
//         (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
//         "nvs_display_cf"
//     );

//     if (!part) {
//         ESP_LOGE(TAG, "Partition not found!");
//         return ESP_ERR_NOT_FOUND;
//     }

//     err = esp_partition_erase_range(part, DEMO_DISPLAY_OFFSET, DEMO_DISPLAY_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     err = esp_partition_write(part, DEMO_DISPLAY_OFFSET, demo_display, DEMO_DISPLAY_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     return err;
// }

// /**
//  * @brief 將顯示器清單寫入 NVS 分區
//  * @param display_list 指向要寫入的顯示器清單的指標
//  * @return esp_err_t 操作結果
//  */
// esp_err_t nvs_write_display_list(char *display_list) {
//     esp_err_t err;

//     const esp_partition_t* part = esp_partition_find_first(
//         (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
//         (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
//         "nvs_display_cf"
//     );

//     if (!part) {
//         ESP_LOGE(TAG, "Partition not found!");
//         return ESP_ERR_NOT_FOUND;
//     }

//     err = esp_partition_erase_range(part, DISPLAY_LIST_OFFSET, DISPLAY_LIST_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     err = esp_partition_write(part, DISPLAY_LIST_OFFSET, display_list, DISPLAY_LIST_MAX_BYTE);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     return err;
// }   

/**
 * @brief 擦除 NVS 中的顯示器配置和清單
 * @return esp_err_t 操作結果
 */
esp_err_t nvs_erase_display_config_list(void) {
    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)MY_NVS_PARTITION_TYPE,
        (esp_partition_subtype_t)MY_NVS_PARTITION_SUBTYPE,
        "nvs_display_cf"
    );

    if (!part) {
        ESP_LOGE(TAG, "Partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Erasing partition at offset 0x%08X, size: %d bytes", (unsigned int)part->address, (unsigned int) (part->size));

    esp_err_t err = esp_partition_erase_range(part, 0, part->size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Partition erased successfully");
    }

    return err;
}
