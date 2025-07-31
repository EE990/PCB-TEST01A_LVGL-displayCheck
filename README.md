# ESP32 LVGL Multi-Panel LCD Display Project

[English](#english) | [繁體中文](#繁體中文)

---

## English

### Project Overview

This is a multi-panel LCD display project based on ESP32 and LVGL graphics library, supporting various LCD panel models. The project uses the ESP-IDF development framework to provide a complete hardware abstraction layer and graphical user interface.

### Features

- **Multi-panel Support**: Supports various LCD panel models
  - 240320W-C001 (ST7789P3 driver chip)
  - XF024QV16A (ST7789P3 driver chip)
  - XF024QV06A (Jd9853 driver chip)
  - (continuously adding)
- **LVGL Graphics Interface**: Uses LVGL 9.3.0 version to provide modern graphical user interface
- **NVS Configuration Storage**: Uses NVS (Non-Volatile Storage) to store display configurations
- **Multi-interface Support**: Supports SPI, QSPI, I8080 and other different interfaces

### Hardware Requirements

#### Supported Development Boards
- ESP32-S3 development board
- Other ESP32 series development boards compatible with ESP-IDF

#### Supported LCD Panels
| Panel Model | Driver Chip | Resolution | Interface | Touch |
|-------------|-------------|------------|-----------|-------|
| 240320W-C001 | ST7789P3 | 320x240 | SPI | No |
| XF024QV16A | ST7789P3 | 320x240 | I8080 | No |
| XF024QV06A | Jd9853 | 320x240 | QSPI | No |

### Installation

#### Prerequisites
- ESP-IDF v5.2.1 or newer
- Python 3.7 or newer
- Git

### Usage

#### Basic Operations
1. After power on, the system will automatically initialize the LCD panel based on selection
2. If long press button 2 before startup, you can enter configuration mode
3. After selection, display information will be shown
4. After restart, the LCD panel will be initialized

#### Button Functions
- **Button 1**: Switch display screens
- **Button 2**: Turn display on/off, long press during startup to enter configuration mode
- **Button 3**: Backlight on/off

### Project Structure

```
PCB-TEST01A_LVGL-displayCheck/
├── components/                 # Hardware components
│   ├── 240320W-C001/         # 240320W-C001 LCD panel driver
│   ├── XF024QV16A/           # XF024QV16A LCD panel driver
│   ├── XF024QV06A/           # XF024QV06A LCD panel driver
│   └── nvs_display_config/   # NVS configuration management
├── main/                      # Main program
│   ├── main.c                # Main program file
│   └── ui/                   # LVGL UI files
│       ├── screens/          # Screen definitions
│       ├── components/       # UI components
│       ├── images/           # Image resources
│       └── fonts/            # Font files
├── CMakeLists.txt            # CMake build file
└── README.md                 # Project documentation
```

### Development Guide

#### Adding New LCD Panels
1. Create a new panel driver directory under `components/`
2. Implement panel initialization functions
3. Add panel information to the `lcd_info[]` array in `main.c`
4. Update configuration options

#### Customizing UI
1. Modify screen files in `main/ui/screens/`
2. Add new UI components to `main/ui/components/`
3. Update event handling functions

---

## 繁體中文

### 專案概述

這是一個基於 ESP32 和 LVGL 圖形庫的多面板 LCD 顯示專案，支援多種不同型號的 LCD 面板。專案使用 ESP-IDF 開發框架，提供完整的硬體抽象層和圖形化使用者介面。

### 功能特色

- **多面板支援**：支援多種 LCD 面板型號
  - 240320W-C001 (ST7789P3 驅動晶片)
  - XF024QV16A (ST7789P3 驅動晶片)
  - XF024QV06A (Jd9853 驅動晶片)
  - (持續增加)
- **LVGL 圖形介面**：使用 LVGL 9.3.0 版本提供現代化的圖形使用者介面
- **NVS 配置儲存**：使用 NVS (Non-Volatile Storage) 儲存顯示配置
- **多介面支援**：支援 SPI、QSPI、I8080 等不同介面

### 硬體需求

#### 支援的開發板
- ESP32-S3 開發板
- 其他相容 ESP-IDF 的 ESP32 系列開發板

#### 支援的 LCD 面板
| 面板型號 | 驅動晶片 | 解析度 | 介面 | 觸控 |
|---------|---------|--------|------|------|
| 240320W-C001 | ST7789P3 | 320x240 | SPI | 無 |
| XF024QV16A | ST7789P3 | 320x240 | I8080 | 無 |
| XF024QV06A | Jd9853 | 320x240 | QSPI | 無 |

### 安裝說明

#### 前置需求
- ESP-IDF v5.2.1 或更新版本
- Python 3.7 或更新版本
- Git

### 使用方法

#### 基本操作
1. 上電後，系統會更具選擇自動初始化 LCD 面板
2. 如果再開機前，長按按鈕2可進入配置模式
3. 選擇後，會顯示顯示器訊息
4. 重開後即會初始化 LCD 面板

#### 按鈕功能
- **按鈕 1**：切換顯示畫面
- **按鈕 2**：開關顯示器，開機時長按進入配置模式
- **按鈕 3**：背光開關

### 專案結構

```
PCB-TEST01A_LVGL-displayCheck/
├── components/                 # 硬體元件
│   ├── 240320W-C001/         # 240320W-C001 LCD 面板驅動
│   ├── XF024QV16A/           # XF024QV16A LCD 面板驅動
│   ├── XF024QV06A/           # XF024QV06A LCD 面板驅動
│   └── nvs_display_config/   # NVS 配置管理
├── main/                      # 主程式
│   ├── main.c                # 主程式檔案
│   └── ui/                   # LVGL UI 檔案
│       ├── screens/          # 畫面定義
│       ├── components/       # UI 元件
│       ├── images/           # 圖片資源
│       └── fonts/            # 字型檔案
├── CMakeLists.txt            # CMake 建置檔案
└── README.md                 # 專案說明
```

### 開發指南

#### 添加新的 LCD 面板
1. 在 `components/` 目錄下創建新的面板驅動目錄
2. 實現面板初始化函數
3. 在 `main.c` 的 `lcd_info[]` 陣列中添加面板資訊
4. 更新配置選項

#### 自定義 UI
1. 修改 `main/ui/screens/` 中的畫面檔案
2. 添加新的 UI 元件到 `main/ui/components/`
3. 更新事件處理函數

