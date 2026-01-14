#include "oled_ssd1306.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "OLED";

// SSD1306 I2C address
#define SSD1306_I2C_ADDRESS 0x3C
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_TIMEOUT_MS 1000

// SSD1306 Commands
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22

// Display buffer
static uint8_t display_buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
static bool oled_initialized = false;

// Simple 5x7 font (ASCII 32-90)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0 (48)
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A (65)
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z (90)
};

static esp_err_t oled_command(uint8_t cmd) {
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, 0x00, true);  // Command mode
    i2c_master_write_byte(i2c_cmd, cmd, true);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, i2c_cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(i2c_cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C command failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

void oled_init(uint8_t sda_pin, uint8_t scl_pin) {

    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Test I2C communication
    i2c_cmd_handle_t test_cmd = i2c_cmd_link_create();
    i2c_master_start(test_cmd);
    i2c_master_write_byte(test_cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(test_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, test_cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(test_cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED not found at address 0x%02X: %s", SSD1306_I2C_ADDRESS, esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "OLED found at 0x%02X, sending initialization commands", SSD1306_I2C_ADDRESS);
    
    // Initialization sequence
    oled_command(SSD1306_DISPLAYOFF);
    oled_command(SSD1306_SETDISPLAYCLOCKDIV);
    oled_command(0x80);
    oled_command(SSD1306_SETMULTIPLEX);
    oled_command(SCREEN_HEIGHT - 1);
    oled_command(SSD1306_SETDISPLAYOFFSET);
    oled_command(0x00);
    oled_command(SSD1306_SETSTARTLINE | 0x00);
    oled_command(SSD1306_CHARGEPUMP);
    oled_command(0x14);
    oled_command(SSD1306_MEMORYMODE);
    oled_command(0x00);
    oled_command(SSD1306_SEGREMAP | 0x01);
    oled_command(SSD1306_COMSCANDEC);
    oled_command(SSD1306_SETCOMPINS);
    oled_command(0x12);
    oled_command(SSD1306_SETCONTRAST);
    oled_command(0xCF);
    oled_command(SSD1306_SETPRECHARGE);
    oled_command(0xF1);
    oled_command(SSD1306_SETVCOMDETECT);
    oled_command(0x40);
    oled_command(SSD1306_DISPLAYALLON_RESUME);
    oled_command(SSD1306_NORMALDISPLAY);
    oled_command(SSD1306_DISPLAYON);
    
    oled_initialized = true;
}

void oled_clear() {
    memset(display_buffer, 0x00, sizeof(display_buffer));
}

void oled_display() {
    if (!oled_initialized) {
        ESP_LOGW(TAG, "OLED not initialized");
        return;
    }
    
    oled_command(SSD1306_COLUMNADDR);
    oled_command(0);
    oled_command(SCREEN_WIDTH - 1);
    oled_command(SSD1306_PAGEADDR);
    oled_command(0);
    oled_command(7);
    
    // Send display buffer in chunks
    for (uint16_t i = 0; i < sizeof(display_buffer); i += 16) {
        i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
        i2c_master_start(i2c_cmd);
        i2c_master_write_byte(i2c_cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(i2c_cmd, 0x40, true);  // Data mode
        
        uint8_t chunk_size = 16;
        if (i + 16 > sizeof(display_buffer)) {
            chunk_size = sizeof(display_buffer) - i;
        }
        
        i2c_master_write(i2c_cmd, &display_buffer[i], chunk_size, true);
        i2c_master_stop(i2c_cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, i2c_cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
        i2c_cmd_link_delete(i2c_cmd);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Display update failed at chunk %d: %s", i/16, esp_err_to_name(ret));
            return;
        }
    }
}

void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    
    if (color) {
        display_buffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y & 7));
    } else {
        display_buffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y & 7));
    }
}

void oled_draw_char(uint8_t x, uint8_t y, char c) {
    if (c < 32 || c > 90) c = 32;
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = font5x7[c - 32][i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                oled_set_pixel(x + i, y + j, 1);
            }
        }
    }
}

void oled_print(uint8_t x, uint8_t y, const char* str) {
    uint8_t pos = x;
    while (*str) {
        oled_draw_char(pos, y, *str);
        pos += 6;
        if (pos >= SCREEN_WIDTH) break;
        str++;
    }
}

void oled_print_number(uint8_t x, uint8_t y, int num) {
    char buffer[12];
    sprintf(buffer, "%d", num);
    oled_print(x, y, buffer);
}

void oled_print_float(uint8_t x, uint8_t y, float num, int decimals) {
    char buffer[20];
    char format[10];
    sprintf(format, "%%.%df", decimals);
    sprintf(buffer, format, num);
    oled_print(x, y, buffer);
}

void oled_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        oled_set_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t i = x; i < x + w && i < SCREEN_WIDTH; i++) {
        oled_set_pixel(i, y, color);
        if (y + h - 1 < SCREEN_HEIGHT) {
            oled_set_pixel(i, y + h - 1, color);
        }
    }
    for (uint8_t i = y; i < y + h && i < SCREEN_HEIGHT; i++) {
        oled_set_pixel(x, i, color);
        if (x + w - 1 < SCREEN_WIDTH) {
            oled_set_pixel(x + w - 1, i, color);
        }
    }
}

void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t i = x; i < x + w && i < SCREEN_WIDTH; i++) {
        for (uint8_t j = y; j < y + h && j < SCREEN_HEIGHT; j++) {
            oled_set_pixel(i, j, color);
        }
    }
}