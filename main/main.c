/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/////************************************************************** */
/////
/////   
/////
////*************************************************************** */

#define CONFIG_USE_LCD_240320W_C001 1
#define CONFIG_USE_LCD_XF024QV16A 0
#define CONFIG_USE_LCD_XF024QV06A 0

#if (CONFIG_USE_LCD_240320W_C001==1)
//#include "esp_lcd_panel_240320W-C001.h"
#define LCD_RST_PIN EXAMPLE_LCD_GPIO_SPI_RST
#define ESP_LCD_NEW_PANEL_XXX_FUNC esp_lcd_new_panel_240320WC001(&lcd_io, &panel_config, &lcd_panel,_240320WC001_INTERFACE_SPI,EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t))
#elif (CONFIG_USE_LCD_XF024QV16A==1)
//#include "esp_lcd_panel_XF024QV16A.h"
#define LCD_RST_PIN CONFIG_ESP32_I8080_PIN_RST
#define ESP_LCD_NEW_PANEL_XXX_FUNC esp_lcd_new_panel_xf024qv16a(&lcd_io, &panel_config, &lcd_panel,EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t))
#elif (CONFIG_USE_LCD_XF024QV06A==1)
//#include "esp_lcd_panel_XF024QV06A.h"
#define LCD_RST_PIN EXAMPLE_LCD_GPIO_SPI_RST
#define ESP_LCD_NEW_PANEL_XXX_FUNC esp_lcd_new_panel_xf024qv06a(&lcd_io, &panel_config, &lcd_panel,XF024QV06A_INTERFACE_QSPI,EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t))
#endif

#include "esp_lcd_panel_240320W-C001.h"
#include "esp_lcd_panel_XF024QV16A.h"
#include "esp_lcd_panel_XF024QV06A.h"
#include "esp_lcd_panel_XF024QV04B.h"

#include "stdlib.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "ui/ui.h"

#include "nvs_display_config.h"

/***************************************** LCD 設定 ****************************************/

/* LCD settings */
#define EXAMPLE_LCD_BITS_PER_PIXEL  (16)

#ifndef CONFIG_DRAW_DEMO_IMG_SOURCE
#define CONFIG_DRAW_DEMO_IMG_SOURCE 0 //0:use RAM(draw by funation) ,1:use ROM
#endif // !CONFIG_DRAW_DEMO_IMG_SOURCE

#define EXAMPLE_LCD_GPIO_SPI_BL         (GPIO_NUM_1)
#define EXAMPLE_LCD_GPIO_LVCD_EN         (GPIO_NUM_15)// !!!原本的 I8080 的 I8080_WR1,改為 LVCD_EN
/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static int BK_light_state=1;

/***************************************** 自訂 ****************************************/
#define DEFAULT_DISPLAY "240320W_C001"

typedef struct 
{
    char *name;
    char *driver_IC_name;
    char *touch_IC_name;
    char *notes;
    esp_err_t (*init_lcd_single_interface)(esp_lcd_panel_io_handle_t *lcd_io, esp_lcd_panel_dev_config_t *panel_config, esp_lcd_panel_handle_t *lcd_panel, int buffer_size);
    esp_err_t (*init_lcd_multi_interface)(esp_lcd_panel_io_handle_t *lcd_io, esp_lcd_panel_dev_config_t *panel_config, esp_lcd_panel_handle_t *lcd_panel, int interface, int buffer_size);
    int lcd_rst_pin;
    int interface;
    int h_res;
    int v_res;
    int color_space;
    int bits_per_pixel;
    int draw_buff_height;
    int draw_buff_double;
    int bl_on_level;
    float bl_voltage;//V
    int bl_current;//mA
}_lcd_info_t;

//目前 bits_per_pixel 只設定 16bit
const _lcd_info_t lcd_info[] = {
    {"240320W_C001"     , "ST7789P3", "null","SPI介面"              , NULL                           , esp_lcd_new_panel_240320WC001, EXAMPLE_LCD_GPIO_SPI_RST  , _240320WC001_INTERFACE_SPI, 320, 240, ESP_LCD_COLOR_SPACE_RGB, EXAMPLE_LCD_BITS_PER_PIXEL, 50, 1, 1, 3.1, 120},
    {"240320W_C001_test", "ST7789P3", "null","測試用,SPI介面"        , NULL                           , esp_lcd_new_panel_240320WC001, EXAMPLE_LCD_GPIO_SPI_RST  , _240320WC001_INTERFACE_SPI, 240, 160, ESP_LCD_COLOR_SPACE_RGB, EXAMPLE_LCD_BITS_PER_PIXEL, 50, 1, 1, 3.1, 120},
    {"XF024QV16A"       , "ST7789P3", "null","I8080介面"            , esp_lcd_new_panel_xf024qv16a,  esp_lcd_new_panel_xf024qv16a   , CONFIG_ESP32_I8080_PIN_RST, -1,                         320, 240, ESP_LCD_COLOR_SPACE_RGB, EXAMPLE_LCD_BITS_PER_PIXEL, 50, 1, 1, 6, 80},
    {"XF024QV06A"       , "Jd9853"  , "null","QSPI介面"             , NULL                           ,esp_lcd_new_panel_xf024qv06a  , EXAMPLE_LCD_GPIO_SPI_RST  , XF024QV06A_INTERFACE_QSPI,  320, 240, ESP_LCD_COLOR_SPACE_RGB, EXAMPLE_LCD_BITS_PER_PIXEL, 50, 1, 1, 3, 80},
    {"XF024QV04B(SPI)"  , "ST7789V" , "null","SPI介面"              , NULL                           ,esp_lcd_new_panel_xf024qv04b  , EXAMPLE_LCD_GPIO_SPI_RST  , XF024QV04B_INTERFACE_SPI,  320, 240, ESP_LCD_COLOR_SPACE_RGB, EXAMPLE_LCD_BITS_PER_PIXEL, 50, 1, 1, 3, 80},
    {"end"              , "null"    , "null",NULL , -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},   //null 用於結尾
};

_lcd_info_t lcd_info_now={0};

display_config_t display_config={
    .default_display={0},
    .demo_display={0},
    .display_list={"240320W_C001\n"
                    "240320W_C001_test\n"
                    "XF024QV16A\n"
                    "XF024QV06A\n"
                    "XF024QV04B(SPI)\n"
                    "null\n"
                    "null\n"
                    "null\n"
                    "null"}
};

static lv_obj_t *demo_frameLine;

bool is_demo_display_unsupport=false;

#if 0 
### for XDT-S80 SPI
#define BUS_SPI_MOSI_GPIO 23
#define BUS_SPI_SCLK_GPIO 18
#define BUS_SPI_CS_GPIO 5
#define BUS_DC_GPIO 21
#define BUS_RESET_GPIO 22
#define BUS_BL_GPIO -1 //1

### for XDT-S80 S3 SPI
#define BUS_SPI_MOSI_GPIO 11
#define BUS_SPI_SCLK_GPIO 12
#define BUS_SPI_CS_GPIO 10
#define BUS_DC_GPIO 21
#define BUS_RESET_GPIO 17//20
#define BUS_BL_GPIO -1 //1
#define BUS_QSPI_IO0   BUS_SPI_MOSI_GPIO //mosi_io_num
#define BUS_QSPI_IO1   (GPIO_NUM_13) //miso_io_num
#define BUS_QSPI_IO2   (GPIO_NUM_14) //quadwp_io_num
#define BUS_QSPI_IO3   (GPIO_NUM_9) //quadhd_io_num
#endif

static const char *TAG = "EXAMPLE";

/***************************************** 按鈕相關設定 ****************************************/
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BUTTON1_GPIO GPIO_NUM_4  // 按键连接到 GPIO4
    #define BUTTON2_GPIO GPIO_NUM_5  // 按键连接到 GPIO5
    #define BUTTON3_GPIO GPIO_NUM_6  // 按键连接到 GPIO6
#elif defined(CONFIG_IDF_TARGET_ESP32)
    #define BUTTON1_GPIO GPIO_NUM_39  // 按键连接到 GPIO39
    #define BUTTON2_GPIO GPIO_NUM_34  // 按键连接到 GPIO34
    #define BUTTON3_GPIO GPIO_NUM_35  // 按键连接到 GPIO35
#endif

#define BUTTON1_FUNCTION_COUNT_MAX 14//12
static int LCD_power_mode=1;
static int screen1_button1_function_count=1;
bool button1FunEn_img1=true;
bool button1FunEn_img2=0;
bool button1FunEn_img3=0;
bool button1FunEn_img4=true;
bool button1FunEn_img5=0;
bool button1FunEn_img6=0;
bool button1FunEn_colorBar=true;
bool button1FunEn_white=true;
bool button1FunEn_black=true;
bool button1FunEn_red=true;
bool button1FunEn_green=true;
bool button1FunEn_blue=true;
bool button1FunEn_net=true;
bool button1FunEn_frameLine=true;
//bool button1FunEn_exword1=0;
//bool button1FunEn_halfVop=0;
#define DEMO_IMG1_NAME ui_img_689278962
#define DEMO_IMG2_NAME ui_img_799870042
#define DEMO_IMG3_NAME ui_img_799871067
#define DEMO_IMG4_NAME ui_img_799872092
#define DEMO_IMG5_NAME ui_img_689278962
#define DEMO_IMG6_NAME ui_img_689278962

static bool button3_state;


/***************************************** LVGL display ****************************************/
static lv_display_t *lvgl_disp = NULL;

#define DRAW_CANVAS_HIGH_MAX 16//16行
static lv_obj_t *canvas_row = NULL;
#if EXAMPLE_LCD_BITS_PER_PIXEL<=16
lv_color16_t *canvas_row_buffer;//static lv_color16_t canvas_row_buffer[EXAMPLE_LCD_V_RES * DRAW_CANVAS_HIGH_MAX];//EXAMPLE_LCD_BITS_PER_PIXEL/8 須整除.否則+1
#elif 
lv_color_t *canvas_row_buffer;//static lv_color_t canvas_row_buffer[EXAMPLE_LCD_V_RES * DRAW_CANVAS_HIGH_MAX];//EXAMPLE_LCD_BITS_PER_PIXEL/8 須整除.否則+1
#endif
lv_obj_t **img_screen_row;//static lv_obj_t *img_screen_row[EXAMPLE_LCD_H_RES/DRAW_CANVAS_HIGH_MAX];
#define HIDE_CANVAS_ROW() do{ for(int a=0;a<lcd_info_now.h_res/DRAW_CANVAS_HIGH_MAX;a++){lv_obj_add_flag(img_screen_row[a], LV_OBJ_FLAG_HIDDEN);} }while(0);
#define SHOW_CANVAS_ROW() do{ for(int a=0;a<lcd_info_now.h_res/DRAW_CANVAS_HIGH_MAX;a++){lv_obj_clear_flag(img_screen_row[a], LV_OBJ_FLAG_HIDDEN);} }while(0);
#define HIDE_IMG() do{ lv_obj_add_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN); }while(0);
#define SHOW_IMG() do{ lv_obj_clear_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN); }while(0);
#define HIDE_FILL_LABEL() do{ lv_obj_add_flag(ui_Label_msg, LV_OBJ_FLAG_HIDDEN); }while(0);
#define SHOW_FILL_LABEL() do{ lv_obj_clear_flag(ui_Label_msg, LV_OBJ_FLAG_HIDDEN); }while(0);

/***************************************** LCD touch ****************************************/
#if LCD_TOUCH_ENABLE
#include "esp_lcd_touch_tt21100.h"

/* Touch settings */
#define EXAMPLE_TOUCH_I2C_NUM       (0)
#define EXAMPLE_TOUCH_I2C_CLK_HZ    (400000)

/* LCD touch pins */
#define EXAMPLE_TOUCH_I2C_SCL       (GPIO_NUM_18)
#define EXAMPLE_TOUCH_I2C_SDA       (GPIO_NUM_8)
#define EXAMPLE_TOUCH_GPIO_INT      (GPIO_NUM_3)
static esp_lcd_touch_handle_t touch_handle = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;
#endif

_lcd_info_t* get_lcd_info(char *name);
void screen1_button1_function(int *function_count);
void screen1_button2_function(void);
void screen1_button3_function(void);
void screen2_button1_function(void);
void screen2_button2_function(void);
void screen2_button3_function(void);

static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD backlight */
    // if(EXAMPLE_LCD_GPIO_BL>0)
    // {
    //     gpio_config_t bk_gpio_config = {
    //         .mode = GPIO_MODE_OUTPUT,
    //         .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL
    //     };
    //     ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    // }

    /* LCD initialization */
    // ESP_LOGD(TAG, "Install LCD driver");
    // const esp_lcd_panel_dev_config_t panel_config = {
    //     .reset_gpio_num = LCD_RST_PIN,
    //     .color_space = EXAMPLE_LCD_COLOR_SPACE,
    //     .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
    // };
    // ESP_GOTO_ON_ERROR(ESP_LCD_NEW_PANEL_XXX_FUNC, err, TAG, "New panel failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = lcd_info_now.lcd_rst_pin,
        .color_space = lcd_info_now.color_space,
        .bits_per_pixel = lcd_info_now.bits_per_pixel,
    };
    if(lcd_info_now.init_lcd_single_interface)
    {
        ESP_GOTO_ON_ERROR(lcd_info_now.init_lcd_single_interface(&lcd_io, &panel_config, &lcd_panel,lcd_info_now.h_res * lcd_info_now.draw_buff_height * sizeof(uint16_t)), err, TAG, "New panel failed");
    }
    else
    {
        ESP_GOTO_ON_ERROR(lcd_info_now.init_lcd_multi_interface(&lcd_io, &panel_config, &lcd_panel,lcd_info_now.interface,lcd_info_now.h_res * lcd_info_now.draw_buff_height * sizeof(uint16_t)), err, TAG, "New panel failed");
    }
    
    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* LCD backlight on */
    //ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL));

    return ret;

err:
    if (lcd_panel) {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io) {
        esp_lcd_panel_io_del(lcd_io);
    }
    #if defined(EXAMPLE_LCD_SPI_NUM)
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    #endif
    return ret;
}

#if LCD_TOUCH_ENABLE
static esp_err_t app_touch_init(void)
{
    /* Initilize I2C */
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_TOUCH_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = EXAMPLE_TOUCH_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = EXAMPLE_TOUCH_I2C_CLK_HZ
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(EXAMPLE_TOUCH_I2C_NUM, &i2c_conf), TAG, "I2C configuration failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(EXAMPLE_TOUCH_I2C_NUM, i2c_conf.mode, 0, 0, 0), TAG, "I2C initialization failed");

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = lcd_info_now.h_res,
        .y_max = lcd_info_now.v_res,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = EXAMPLE_TOUCH_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_TT21100_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)EXAMPLE_TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_tt21100(tp_io_handle, &tp_cfg, &touch_handle);
}
#endif

static esp_err_t app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,         /* LVGL task priority */
        .task_stack = 4096,         /* LVGL task stack size */
        .task_affinity = -1,        /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5        /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = lcd_info_now.h_res * lcd_info_now.draw_buff_height,
        .double_buffer = lcd_info_now.draw_buff_double,
        .hres = lcd_info_now.h_res,
        .vres = lcd_info_now.v_res,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = true,//false,
            .mirror_x = false,//true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

#if LCD_TOUCH_ENABLE
    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
#endif

    return ESP_OK;
}

static void app_main_display(void)
{
    /* Task lock */
    lvgl_port_lock(0);

    /* Your LVGL objects code here .... */
    ui_init();
    lv_disp_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_90);

    //***********  ui_Screen1  ***********
    //*** init lvgl canvas for draw
    //分配記憶體給 img_screen_row 和 canvas_row_buffer
    img_screen_row = malloc(sizeof(lv_obj_t*) * (lcd_info_now.h_res / DRAW_CANVAS_HIGH_MAX));
    canvas_row_buffer = malloc(sizeof(typeof(*canvas_row_buffer)) * lcd_info_now.v_res * DRAW_CANVAS_HIGH_MAX);
    //建立畫布
    canvas_row=lv_canvas_create(NULL);
    lv_canvas_set_buffer(canvas_row,canvas_row_buffer,lcd_info_now.v_res,DRAW_CANVAS_HIGH_MAX,LV_COLOR_FORMAT_RGB565) ;
    lv_canvas_fill_bg(canvas_row,lv_color_make(0,0,0),LV_OPA_0);
    for(int i=0;i<lcd_info_now.h_res/DRAW_CANVAS_HIGH_MAX;i++)//創造數個圖片,來源相同
    {
        img_screen_row[i]=lv_image_create(ui_Screen1);
        lv_img_set_src( img_screen_row[i], lv_canvas_get_image(canvas_row));
        lv_obj_set_pos( img_screen_row[i], 0, i*DRAW_CANVAS_HIGH_MAX);
    }

    //建立frameLine
    demo_frameLine = lv_line_create(ui_Screen1);                  // 建立 line 物件
    lv_obj_set_style_line_width(demo_frameLine, 2, 0);            // 線寬
    lv_obj_set_style_line_color(demo_frameLine, lv_palette_main(LV_PALETTE_RED), 0);  // 線色
    lv_obj_set_style_line_rounded(demo_frameLine, true, 0);      // 不要圓角
    //lv_obj_align(demo_frameLine, LV_ALIGN_TOP_LEFT, 0, 0);        // 對齊左上（可調整位置）

    if(is_demo_display_unsupport)
    {
        //    !!!!!顯示錯誤訊息!!!!!
        //   label 沒法設置上下對齊
        //   所以設置了容器 ui_Label_msg 
        //   裡面設置 label ui_Label_msgText 來顯示錯誤訊息
        //   上下對齊用 lv_obj_center() 來設置
        lv_obj_set_width(ui_Label_msg, lcd_info_now.v_res);//設置寬度
        lv_obj_set_height(ui_Label_msg, lcd_info_now.h_res);//設置高度
        lv_obj_set_style_bg_color(ui_Label_msg, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);//設置背景色
        lv_obj_set_style_bg_opa(ui_Label_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);//設置不透明
        lv_obj_set_style_text_align(ui_Label_msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);//設置文字水平置中
        
        lv_obj_center(ui_Label_msgText);
        lv_obj_set_width(ui_Label_msgText, lcd_info_now.v_res);//設置寬度,
        //lv_obj_set_height(ui_Label_msgText, lcd_info_now.h_res);//不可設定高度
        //無法使用 sprintf 來設置文字
        char error_msg[100]={0};
        char *error_msg_start=error_msg;
        memcpy(error_msg_start, "所選測試顯示器不支援: ", sizeof("所選測試顯示器不支援: "));
        error_msg_start+=sizeof("所選測試顯示器不支援: ");
        if(*error_msg_start=='\0')  error_msg_start--;//如果字串結尾,則往前一個字元
        memcpy(error_msg_start, display_config.demo_display, sizeof(display_config.demo_display));
        error_msg_start+=sizeof(display_config.demo_display);
        error_msg[99]='\0';
        lv_label_set_text(ui_Label_msgText, error_msg);//設置文字

        lv_obj_clear_flag(ui_Label_msg, LV_OBJ_FLAG_HIDDEN);//設置顯示
        lv_obj_move_foreground(ui_Label_msg);//設置置頂
    }
    else
    {
        lv_obj_add_flag(ui_Label_msg, LV_OBJ_FLAG_HIDDEN);
    }

    //第1張圖片
    int count=0;
    screen1_button1_function(&count);

    //***********  ui_Screen2  ***********
    //設定 roller
    lv_roller_set_options(ui_rollerDisplayList, display_config.display_list, LV_ROLLER_MODE_INFINITE);
    //定位初始 roller 選項
    for(int selected_index=0;selected_index<lv_roller_get_option_cnt(ui_rollerDisplayList);selected_index++)
    {
        lv_roller_set_selected(ui_rollerDisplayList, selected_index, LV_ANIM_OFF);
        char display_name[DEMO_DISPLAY_MAX_BYTE];
        lv_roller_get_selected_str(ui_rollerDisplayList, display_name, sizeof(display_name));
        if(strcmp(display_config.demo_display, display_name)==0)
        {
            break;
        }
    }

    //設定 textarea 的寬度,高度,隱藏
    lv_obj_set_width(ui_textarea_msg, lcd_info_now.v_res);
    lv_obj_set_height(ui_textarea_msg, lcd_info_now.h_res);
    lv_obj_add_flag(ui_textarea_msg, LV_OBJ_FLAG_HIDDEN);

    //切換畫面
    // lv_scr_load_anim(ui_Screen1,
    //              LV_SCR_LOAD_ANIM_MOVE_RIGHT,  // 切換動畫類型
    //              300,                          // 動畫時間 (ms)
    //              0,                            // 延遲
    //              false);                       // true = 刪除前一畫面

    /* Task unlock */
    lvgl_port_unlock();
}

/**
 * @brief 初始化 NVS,只有沒設置過的 flash 區段才會寫入預設值
 * @note display_config->display_list 宣告時給與的值會被當成預設值
 * @note                              如果 flash 沒設置過,則會被一起寫入
 * @note                              如果 flash 被設置過,則會在讀取時被覆蓋
 */
void get_or_setDefault_nvs(void)
{
    // 讀取 NVS 中的顯示器清單
    nvs_flash_init(); // 初始化預設 nvs 分區
    nvs_flash_init_partition("nvs_display_cf");
    // nvs_erase_display_config_list();//清除 flash 區段,清除後會寫入預設值

    // 讀取 NVS 中的顯示器清單,如果沒有則寫入預設值
    nvs_read_default_display(display_config.default_display);
    if(display_config.default_display[0] == 0xff && display_config.default_display[1] == 0xff && display_config.default_display[2] == 0xff) //只判讀 default_display 判斷是否沒設置過
    {
        ESP_LOGW("nvs_display_config","No default display config found.write new default config");
        memset(display_config.default_display, 0, DEFAULT_DISPLAY_MAX_BYTE);
        memcpy(display_config.default_display, DEFAULT_DISPLAY, sizeof(DEFAULT_DISPLAY));
        memset(display_config.demo_display, 0, DEMO_DISPLAY_MAX_BYTE);
        memcpy(display_config.demo_display, DEFAULT_DISPLAY, sizeof(DEFAULT_DISPLAY));
        //memcpy(display_config.display_list, (char*)display_list,DISPLAY_LIST_MAX_BYTE);
        nvs_write_display_config(&display_config);
    }

    //再次讀取 NVS 中的顯示器清單
    memset(&display_config, 0, sizeof(display_config_t));
    nvs_read_display_config(&display_config);   
}

/**
 * @brief 取得 lcd_info 的指標
 * @param name 顯示器名稱
 * @return lcd_info 的指標,如果沒找到則回傳 NULL
 */
_lcd_info_t* get_lcd_info(char *name)
{
    int i=0;
    while(strcmp(lcd_info[i].name, "end")!=0)
    {
        if(strcmp(lcd_info[i].name, name)==0)//找到
        {
            return &lcd_info[i];
        }
        i++;
    }

    return NULL;
}

/**
 * @brief 檢測 button2 是否常按,切換到配置頁面
 * @return 是否切換到配置頁面
 * true: 切換到配置頁面
 * false: 不須切換到配置頁面
 */
bool check_button2_long_press(void)
{
    bool ret=false;
    static int button2_long_press_count=0;

    // 檢測 button2 是否常按,切換到頁面2
    while(gpio_get_level(BUTTON2_GPIO)==0)
    {
        vTaskDelay(10/portTICK_PERIOD_MS);
        button2_long_press_count++;
        if(button2_long_press_count>=250)//250*10ms=2.5s
        {
            //********** 確認切換到配置頁面 **********
            ret=true;

            break;
        }
    }

    return ret;       
}

//***** hardware 按鍵 功能 *****/
void screen1_button1_function(int *function_count)
{
    printf("button1.\n");

    if(LCD_power_mode!=0)
    {
        /* Task lock */
        lvgl_port_lock(0);

        HIDE_CANVAS_ROW();//隱藏畫布
        HIDE_IMG();//隱藏圖片
        //HIDE_FILL_LABEL();//隱藏文字
        bool funHasEN=false;
        while(!funHasEN)
        {
            switch (*function_count)
            {
                case 0://*** 圖1
                    if(button1FunEn_img1)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG1_NAME);
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 1://*** 圖2
                    if(button1FunEn_img2)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG2_NAME);
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 2://*** 圖3
                    if(button1FunEn_img3)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG3_NAME); 
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 3://*** 圖4
                    if(button1FunEn_img4)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG4_NAME);
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 4://*** 圖5
                    if(button1FunEn_img5)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG5_NAME);
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 5://*** 圖6
                    if(button1FunEn_img6)
                    {
                        lv_image_set_src(ui_Image2, &DEMO_IMG6_NAME);
                        SHOW_IMG();//顯示圖片
                        funHasEN=true;
                    }
                    break;
                case 6://*** RGB BAR */
                    if(button1FunEn_colorBar)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_rgb_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            int bar_signal_width=lcd_info_now.v_res/3;
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                //行
                                for(int k=0;k<lcd_info_now.v_res;k++)
                                {
                                    if(k/bar_signal_width==0)
                                    {
                                        color.red|=0xff;
                                        color.green=0x00;
                                        color.blue=0x00;
                                    }else if(k/bar_signal_width==1)
                                    {
                                        color.red=0x00;
                                        color.green|=0xff;
                                        color.blue=0x00;
                                    }else if(k/bar_signal_width==2)
                                    {
                                        color.red=0x00;
                                        color.green=0x00;
                                        color.blue|=0xff;
                                    }

                                    canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE  
                        funHasEN=true;
                    }
                    break;
                case 7://*** 白畫面
                    if(button1FunEn_white)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_white_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                    //行
                                    for(int k=0;k<lcd_info_now.v_res;k++)
                                    {
                                        color.blue|=0xff;
                                        color.green|=0xff;
                                        color.red|=0xff;
                                        canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                    }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE
                        funHasEN=true;
                    }
                    break;
                case 8://*** 黑畫面
                    if(button1FunEn_black)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_black_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                    //行
                                    for(int k=0;k<lcd_info_now.v_res;k++)
                                    {
                                        canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                    }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE
                        funHasEN=true;
                    }
                    break;
                case 9://*** 紅畫面
                    if(button1FunEn_red)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_red_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                    //行
                                    for(int k=0;k<lcd_info_now.v_res;k++)
                                    {
                                        color.blue|=0x00;
                                        color.green|=0x00;
                                        color.red|=0xff;
                                        canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                    }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE
                        funHasEN=true;
                    }
                    break;
                case 10://*** 綠畫面
                    if(button1FunEn_green)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_green_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                    //行
                                    for(int k=0;k<lcd_info_now.v_res;k++)
                                    {
                                        color.blue=0x00;
                                        color.green|=0xff;
                                        color.red=0x00;
                                        canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                    }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE
                        funHasEN=true;
                    }
                    break;
                case 11://*** 藍畫面
                    if(button1FunEn_blue)
                    {
                        #if CONFIG_DRAW_DEMO_IMG_SOURCE == 1
                            lv_image_set_src(ui_Image2, &ui_img_blue_320x240_png);
                            SHOW_IMG();//顯示圖片
                        #else
                            #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                                lv_color16_t color;
                            #else
                                lv_color_t color;
                            #endif
                            
                            color.blue=color.green=color.red=0;
                            //列
                            for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                            {
                                    //行
                                    for(int k=0;k<lcd_info_now.v_res;k++)
                                    {
                                        color.blue|=0xff;
                                        color.green=0x00;
                                        color.red=0x00;
                                        canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                    }
                            }
                            SHOW_CANVAS_ROW();//顯示畫布
                        #endif // CONFIG_DRAW_DEMO_IMG_SOURCE
                        funHasEN=true;
                    }
                    break;
                case 12://*** 網.斑
                    if(button1FunEn_net)
                    {
                        #if EXAMPLE_LCD_BITS_PER_PIXEL<=16
                            lv_color16_t color;
                        #else
                            lv_color_t color;
                        #endif
                        
                        color.blue=color.green=color.red=0;
                        //列
                        for(int i=0;i<DRAW_CANVAS_HIGH_MAX;i++)
                        {
                                if(i%8==0)
                                {
                                    color.blue^=0xff;
                                    color.green^=0xff;
                                    color.red^=0xff;
                                }

                                //行
                                for(int k=0;k<lcd_info_now.v_res;k++)
                                {
                                    if(k%8==0)
                                    {
                                        color.blue^=0xff;
                                        color.green^=0xff;
                                        color.red^=0xff;
                                    }
                                    canvas_row_buffer[k+(i*lcd_info_now.v_res)]=color;
                                }
                        }
                        SHOW_CANVAS_ROW();//顯示畫布
                        funHasEN=true;
                    }
                    break;
                case 13:
                    if(button1FunEn_frameLine)
                    {
                        static lv_point_t line_points[5]; 
                        line_points[0].x = 0; line_points[0].y = 0;
                        line_points[1].x = 0; line_points[1].y = lcd_info_now.h_res-1;
                        line_points[2].x = lcd_info_now.v_res-1; line_points[2].y = lcd_info_now.h_res-1;
                        line_points[3].x = lcd_info_now.v_res-1; line_points[3].y = 0;
                        line_points[4].x = 0; line_points[4].y = 0;

                        lv_obj_set_style_line_width(demo_frameLine, 8, 0);     // 線寬
                        lv_line_set_points(demo_frameLine, line_points, 5);    // 設定5個點
                        lv_obj_move_background(demo_frameLine);//至底層,
                        lv_obj_set_pos(demo_frameLine, 0, 0);     
                        funHasEN=true;
                    }
                    break;
                case 14:
                    break;
                default://為定義的功能，強制結束
                    (*function_count)=0;
                    break;
            }
            
            //如果沒有選擇功能，則跳到下一個功能
            if(!funHasEN)
            {
                (*function_count)=((*function_count)+1);//% BUTTON1_FUNCTION_COUNT_MAX;
                continue;
            }
        }

        /* Task unlock */
        lvgl_port_unlock();
    }
}

void screen1_button2_function(void)
{
    printf("power down.\n");

    if(LCD_power_mode==1)
    {   
        //關閉背光
        BK_light_state=!lcd_info_now.bl_on_level;
        gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, BK_light_state);

        //關閉LCD
        vTaskDelay(50/portTICK_PERIOD_MS);
        esp_lcd_panel_disp_on_off(lcd_panel, false);//關閉LCD
        LCD_power_mode=0;

        //關閉LVCD_EN
        gpio_set_level(EXAMPLE_LCD_GPIO_LVCD_EN, 0);

        //等待LCD關閉完成
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
    else
    {
        //開啟LVCD_EN
        gpio_set_level(EXAMPLE_LCD_GPIO_LVCD_EN, 1);
        //delay 50ms,等待電壓穩定
        vTaskDelay(50/portTICK_PERIOD_MS);

        //重新初始化LCD
        esp_lcd_panel_reset(lcd_panel);
        esp_lcd_panel_init(lcd_panel);
        esp_lcd_panel_mirror(lcd_panel, true, true);
        esp_lcd_panel_disp_on_off(lcd_panel, true);

        LCD_power_mode=1;

        //顯示圖1
        screen1_button1_function_count=0;
        screen1_button1_function(&screen1_button1_function_count);//顯示圖1

        //開啟背光
        BK_light_state=lcd_info_now.bl_on_level;
        gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, BK_light_state);
    }
}

void screen1_button3_function(void)
{
    printf("BK light on/off.\n");

    if(BK_light_state!=0)//0:off, 1:on
    {
        //lcdBacklightOff();
        BK_light_state=!lcd_info_now.bl_on_level;
    }
    else
    {
        //lcdBacklightOn();
        BK_light_state=lcd_info_now.bl_on_level;
    }
    gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, BK_light_state);
}

void screen2_button1_function(void)
{
    /* Task lock */
    lvgl_port_lock(0);

    //如果 textarea 隱藏,則設定所選顯示器
    if(lv_obj_has_flag(ui_textarea_msg, LV_OBJ_FLAG_HIDDEN))
    {
        // 設定選擇的項目
        int selected_index = lv_roller_get_selected(ui_rollerDisplayList);  
        selected_index++;
        //if(selected_index>=get_rollerDisplayList_option_count())
        if(selected_index>=lv_roller_get_option_cnt(ui_rollerDisplayList))
        {
            selected_index=0;
        }
        lv_roller_set_selected(ui_rollerDisplayList, selected_index, LV_ANIM_ON);

        // 重新重繪
        lv_obj_invalidate(ui_rollerDisplayList);
    }

    /* Task unlock */
    lvgl_port_unlock();
}

void screen2_button2_function(void)
{
    /* Task lock */
    lvgl_port_lock(0);
    
    //如果 textarea 隱藏,則設定所選顯示器
    if(lv_obj_has_flag(ui_textarea_msg, LV_OBJ_FLAG_HIDDEN))
    {
        char display_name[DEMO_DISPLAY_MAX_BYTE];
        lv_roller_get_selected_str(ui_rollerDisplayList, display_name, sizeof(display_name));
        display_name[DEMO_DISPLAY_MAX_BYTE-1]='\0';

        if(strcmp(display_name, display_config.demo_display)!=0)
        {
            memcpy(display_config.demo_display, display_name, DEMO_DISPLAY_MAX_BYTE);
            nvs_write_display_config(&display_config);
            ESP_LOGI(TAG,"save, display_name: %s", display_name);
        }
        else    
        {
            ESP_LOGI(TAG,"no change");
        }

        //顯示所選顯示器資訊    
        char buffer[15];
        _lcd_info_t * lcd_info_demo = get_lcd_info(display_config.demo_display);

        lv_textarea_add_text(ui_textarea_msg, "所選測試顯示器: \n\t");
        lv_textarea_add_text(ui_textarea_msg, lcd_info_demo->name);
        lv_textarea_add_text(ui_textarea_msg, "\n");

        lv_textarea_add_text(ui_textarea_msg, "背光電壓: ");
        // 取小數點前後
        int int_part = (int)lcd_info_demo->bl_voltage;                           // 小數點前
        int frac_part = (int)((lcd_info_demo->bl_voltage)*100) - (int_part*100); // 小數後兩位 *100 
        lv_textarea_add_text(ui_textarea_msg, itoa(int_part, buffer, 10));
        lv_textarea_add_text(ui_textarea_msg, ".");
        lv_textarea_add_text(ui_textarea_msg, itoa(frac_part, buffer, 10));
        lv_textarea_add_text(ui_textarea_msg, " V\n");

        lv_textarea_add_text(ui_textarea_msg, "背光電流: ");
        lv_textarea_add_text(ui_textarea_msg, itoa(lcd_info_demo->bl_current, buffer, 10));
        lv_textarea_add_text(ui_textarea_msg, " mA\n");

        lv_textarea_add_text(ui_textarea_msg, "解析度: ");
        lv_textarea_add_text(ui_textarea_msg, itoa(lcd_info_demo->v_res, buffer, 10));
        lv_textarea_add_text(ui_textarea_msg, " x ");
        lv_textarea_add_text(ui_textarea_msg, itoa(lcd_info_demo->h_res, buffer, 10));
        lv_textarea_add_text(ui_textarea_msg, "\n");

        lv_textarea_add_text(ui_textarea_msg, "驅動 IC: ");
        lv_textarea_add_text(ui_textarea_msg, lcd_info_demo->driver_IC_name);
        lv_textarea_add_text(ui_textarea_msg, "\n");

        if(lcd_info_demo->notes!=NULL)
        {
            lv_textarea_add_text(ui_textarea_msg, "備註: \n\t");
            lv_textarea_add_text(ui_textarea_msg, lcd_info_demo->notes);
            lv_textarea_add_text(ui_textarea_msg, "\n");
        }

        lv_textarea_add_text(ui_textarea_msg, "\n");
        //lv_textarea_add_text(ui_textarea_msg, "!!! 再次按下 button2 重啟 !!!");
        lv_textarea_add_text(ui_textarea_msg, "!!! 請關掉電源後重啟 !!!");

        lv_obj_clear_flag(ui_textarea_msg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_textarea_msg);
    }
    else
    {
        //esp_restart();
    }
    

    /* Task unlock */
    lvgl_port_unlock();
}

void button_task(void * arg)
{
    //********* init button and LED */
    // init button1  gpio
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON1_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf);
    // init button2  gpio
    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << BUTTON2_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf2);

    // init button3  gpio
    gpio_config_t io_conf3 = {
        .pin_bit_mask = (1ULL << BUTTON3_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf3);
    button3_state=gpio_get_level(BUTTON3_GPIO);

    // BUTTON1,BUTTON2 為回彈按鍵, 如事前就按下,則等待按鍵回復 (BUTTON3為常態按鍵)
    while(gpio_get_level(BUTTON1_GPIO)==0 || gpio_get_level(BUTTON2_GPIO)==0)
    {
        vTaskDelay(100/portTICK_PERIOD_MS);
        ESP_LOGI("button task", "wait button");
    }

    ESP_LOGI("button task", "start button task");

    while(1)
    {
        //**************************  hardway button **************************
        // 每 10ms 讀取一次
        // Button1 
        vTaskDelay(5/portTICK_PERIOD_MS); 
        int button_state = gpio_get_level(BUTTON1_GPIO); // 读取 GPIO17 电平
        if (button_state == 0) 
        {
            vTaskDelay(50/portTICK_PERIOD_MS);//防彈跳
            button_state = gpio_get_level(BUTTON1_GPIO); // 再次读取 GPIO17 电平
            if (button_state == 0) 
            {
                // 確認觸發
                if(lv_scr_act()==ui_Screen1)
                {
                    screen1_button1_function_count=(screen1_button1_function_count+1)% BUTTON1_FUNCTION_COUNT_MAX;
                    screen1_button1_function(&screen1_button1_function_count);
                }
                else if(lv_scr_act()==ui_Screen2)
                {
                     screen2_button1_function();
                }

                // 等待按鍵回復
                while(button_state==0)
                {
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    button_state = gpio_get_level(BUTTON1_GPIO); // 再次读取 GPIO17 电平
                }
            }
        }

        // Button2 
        vTaskDelay(5/portTICK_PERIOD_MS); 
        button_state = gpio_get_level(BUTTON2_GPIO); // 读取 GPIO16 电平
        if (button_state == 0) 
        {
            vTaskDelay(50/portTICK_PERIOD_MS);//防彈跳
            button_state = gpio_get_level(BUTTON2_GPIO); // 再次读取 GPIO16 电平
            if (button_state == 0) 
            {
                // 確認觸發
                if(lv_scr_act()==ui_Screen1)
                {
                    screen1_button2_function();
                }
                else if(lv_scr_act()==ui_Screen2)
                {
                    screen2_button2_function();
                }

                // 等待按鍵回復
                while(button_state==0)
                {
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    button_state = gpio_get_level(BUTTON2_GPIO); // 再次读取 GPIO16 电平
                }
            }
        }

         // Button3 
        vTaskDelay(5/portTICK_PERIOD_MS); 
        button_state = gpio_get_level(BUTTON3_GPIO); // 读取 GPIO16 电平
        if (button_state != button3_state) 
        {
            vTaskDelay(50/portTICK_PERIOD_MS);//防彈跳
            button_state = gpio_get_level(BUTTON3_GPIO); // 再次读取 GPIO16 电平
            if (button_state  != button3_state) 
            {
                // 確認觸發
                if(lv_scr_act()==ui_Screen1)
                {
                    screen1_button3_function();
                }
                else if(lv_scr_act()==ui_Screen2)
                {
                    //screen2_button3_function();
                }

                // 更新按鍵狀態
                button3_state=button_state;
            }
        }
    }
}

//********************************** */
// 把 UART0(Debug port) 的 TX.RX,切換為 GPIO
//********************************** */
void switch_uart0_to_gpio() 
{
    ESP_LOGI("GPIO_TX_SWITCH", "切換 TX.RX 為 GPIO1.3");

    // 取消 UART0 對 GPIO1 的控制，避免影響
    uart_driver_delete(UART_NUM_0);

    // 設置 GPIO1 為普通 GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_1),//UART0, TXD 默認為 GPIO1
        .mode = GPIO_MODE_OUTPUT,  // 這裡設為輸入模式，你也可以設為輸出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);


    // 設置 GPIO3 為普通 GPIO
    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_3),//UART0, RXD 默認為 GPIO3
        .mode = GPIO_MODE_OUTPUT,  // 這裡設為輸入模式，你也可以設為輸出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf2);
}

//********************************** */
// 把 GPIO1 切換回 UART0(Debug port) 的 TX(GPIO1)RX(GPIO3)
//********************************** */
void switch_gpio_to_uart0() {
    ESP_LOGI("GPIO_TX_SWITCH", "恢復 GPIO 為 TX.RX");

    // 重新初始化 UART0，讓 GPIO1.3 恢復為 TX.RX
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, GPIO_NUM_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM_0, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

//********** 初始化 GPIO **********
void init_gpio()
{
     // init button1  gpio
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON1_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf);

    // init button2  gpio
    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << BUTTON2_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf2);

    // init button3  gpio
    gpio_config_t io_conf3 = {
        .pin_bit_mask = (1ULL << BUTTON3_GPIO),  // 选择要配置的引脚
        .mode = GPIO_MODE_INPUT,               // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf3);
    button3_state=gpio_get_level(BUTTON3_GPIO);

    // init bk  gpio
    if(EXAMPLE_LCD_GPIO_SPI_BL>0)
    {
        gpio_reset_pin(EXAMPLE_LCD_GPIO_SPI_BL);
        gpio_config_t io_conf3 = {
            .pin_bit_mask = (1ULL << EXAMPLE_LCD_GPIO_SPI_BL),  // 选择要配置的引脚
            .mode = GPIO_MODE_OUTPUT,               // 设置为输入模式
            .pull_up_en = GPIO_PULLUP_DISABLE,      // 禁用内部上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
            .intr_type = GPIO_INTR_DISABLE         // 禁用中断
        };
        gpio_config(&io_conf3);
        gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, 1);//初始狀態固定為1
    }

    // init lvcd_en  gpio
    if(EXAMPLE_LCD_GPIO_LVCD_EN>0)
    {
        gpio_reset_pin(EXAMPLE_LCD_GPIO_LVCD_EN);
        gpio_config_t io_conf4 = {
            .pin_bit_mask = (1ULL << EXAMPLE_LCD_GPIO_LVCD_EN),  // 选择要配置的引脚
            .mode = GPIO_MODE_OUTPUT,               // 设置为输入模式
            .pull_up_en = GPIO_PULLUP_DISABLE,      // 禁用内部上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用内部下拉
            .intr_type = GPIO_INTR_DISABLE         // 禁用中断
        };
        gpio_config(&io_conf4);
        gpio_set_level(EXAMPLE_LCD_GPIO_LVCD_EN, 1);//初始狀態固定為1
    }
}

void app_main(void)
{ 
    //********** 預設第一個顯示的頁面 **********
    lv_obj_t ** ui_first_screen;
    ui_first_screen=&ui_Screen1;

    #if defined(CONFIG_IDF_TARGET_ESP32)
    //********** 切換 UART0 的 TX.RX 為 GPIO **********
    switch_uart0_to_gpio();
    #endif

    //********** 初始化 GPIO **********
    init_gpio();

    //********** 切換 UART0 的 TX.RX 為 GPIO **********
    // switch_gpio_to_uart0();
    printf("\n");

    //********** 初始化 NVS,只有沒設置過的 flash 區段才會寫入預設值 **********
    get_or_setDefault_nvs();
    
    _lcd_info_t * lcd_info_default = get_lcd_info(display_config.default_display);
    _lcd_info_t * lcd_info_demo = get_lcd_info(display_config.demo_display);
    //***** 確認 lcd_info_default 和 lcd_info_demo 是否存在 ******
    //****** 有支援 主顯示器(default_display) 程式才會繼續 ********
    if(lcd_info_default!=NULL && lcd_info_demo!=NULL)//正常情況,使用 副顯示器(demo_display)程式繼續
    {
        memcpy(&lcd_info_now, lcd_info_demo, sizeof(_lcd_info_t));//設置為 demo_display 的值
        ESP_LOGI(TAG,"default_display: %s", display_config.default_display);
        ESP_LOGI(TAG,"demo_display: %s", display_config.demo_display);
    }
    else if(lcd_info_default!=NULL && lcd_info_demo==NULL)//"有"支援 主顯示器(default_display) 但"沒"有支援選中的 副顯示器(demo_display), 使用 主顯示器(default_display) 程式繼續
    {
        memcpy(&lcd_info_now, lcd_info_default, sizeof(_lcd_info_t));//設置為 default_display 的值
        ESP_LOGI(TAG,"default_display: %s", display_config.default_display);
        ESP_LOGW(TAG,"demo_display info not found");
        //需在預設顯示器顯示錯誤訊息
        is_demo_display_unsupport=true;
    }
    else if(lcd_info_default==NULL && lcd_info_demo!=NULL)//"沒"支援 主顯示器(default_display) 但"有"支援選中的 副顯示器(demo_display), 卡死在這裡
    {
        ESP_LOGW(TAG,"default_display info not found");
        ESP_LOGI(TAG,"demo_display: %s", display_config.demo_display);
        while(1)
        {
            static int BK_light_state_temp=0;
            gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, !BK_light_state_temp);
            BK_light_state_temp=!BK_light_state_temp;
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }
    }
    else if(lcd_info_default==NULL && lcd_info_demo==NULL)//"沒"支援 主顯示器(default_display) 也"沒"支援選中的 副顯示器(demo_display), 卡死在這裡
    {
        ESP_LOGW(TAG,"default_display info not found");
        ESP_LOGW(TAG,"demo_display info not found");
        while(1)
        {
            static int BK_light_state_temp=0;
            gpio_set_level(EXAMPLE_LCD_GPIO_SPI_BL, !BK_light_state_temp);
            BK_light_state_temp=!BK_light_state_temp;
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }
    }

    //********** 檢測 button2 是否長按,切換到頁面2 **********
    if(check_button2_long_press())
    {
        ui_first_screen=&ui_Screen2;
        //強制切換為主顯示器
        memcpy(&lcd_info_now, lcd_info_default, sizeof(_lcd_info_t));//設置為 default_display 的值
    }
    else
    {
        ui_first_screen=&ui_Screen1;
    }
    
    //**************************************************************************************************************************************** 
    //************************************** 執行以下程式前,需先確認 lcd_info_now 已設置 ******************************************************* 
    //**************************************************************************************************************************************** 


    //********** LCD HW initialization **********
    ESP_ERROR_CHECK(app_lcd_init());

    #if LCD_TOUCH_ENABLE
    //********** Touch initialization **********
    ESP_ERROR_CHECK(app_touch_init());
    #endif

    //********** LVGL initialization **********
    ESP_ERROR_CHECK(app_lvgl_init());

    //********** 輸出部分 NVS 中的顯示器清單 **********
    // printf("default_display: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", display_config.default_display[0], display_config.default_display[1], display_config.default_display[2], display_config.default_display[3], display_config.default_display[4], display_config.default_display[5], display_config.default_display[6], display_config.default_display[7], display_config.default_display[8], display_config.default_display[9], display_config.default_display[10], display_config.default_display[11], display_config.default_display[12], display_config.default_display[13], display_config.default_display[14]);
    // printf("demo_display: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", display_config.demo_display[0], display_config.demo_display[1], display_config.demo_display[2], display_config.demo_display[3], display_config.demo_display[4], display_config.demo_display[5], display_config.demo_display[6], display_config.demo_display[7], display_config.demo_display[8], display_config.demo_display[9], display_config.demo_display[10], display_config.demo_display[11], display_config.demo_display[12], display_config.demo_display[13], display_config.demo_display[14]);
    // printf("display_list: %x %x %x %x %x %x\n", display_config.display_list[0], display_config.display_list[1], display_config.display_list[2], display_config.display_list[3], display_config.display_list[4], display_config.display_list[5]);

    //********** Show LVGL objects **********
    app_main_display();

    //********** 切換第一個顯示的頁面 **********
    lv_scr_load(*ui_first_screen);
    
    TaskHandle_t buttonTaskHandle;
    xTaskCreate(button_task,"button task",4096,NULL,tskIDLE_PRIORITY,&buttonTaskHandle);  
}
