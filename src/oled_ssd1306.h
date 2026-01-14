#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

typedef struct {
    uint16_t bitmapOffset;  // Pointer into GFXfont->bitmap
    uint8_t  width;         // Bitmap dimensions in pixels
    uint8_t  height;
    uint8_t  xAdvance;      // Distance to advance cursor (x axis)
    int8_t   xOffset;       // X dist from cursor pos to UL corner
    int8_t   yOffset;       // Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
    uint8_t  *bitmap;       // Glyph bitmaps, concatenated
    GFXglyph *glyph;        // Glyph array
    uint8_t   first;        // ASCII extents (first char)
    uint8_t   last;         // ASCII extents (last char)
    uint8_t   yAdvance;     // Newline distance (y axis)
} GFXfont;

// Initialize the OLED display
void oled_init(uint8_t sda_pin, uint8_t scl_pin);

// Clear the display buffer
void oled_clear();

// Update the physical display with buffer contents
void oled_display();

// Set a single pixel
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color);

// Draw a character at position
void oled_draw_char(uint8_t x, uint8_t y, char c);

// Print a string at position
void oled_print(uint8_t x, uint8_t y, const char* str);

// Print an integer at position
void oled_print_number(uint8_t x, uint8_t y, int num);

// Print a float at position with specified decimal places
void oled_print_float(uint8_t x, uint8_t y, float num, int decimals);

// Draw a line
void oled_draw_line(int x0, int y0, int x1, int y1, uint8_t color);

// Draw a rectangle (outline)
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

// Draw a filled rectangle
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

// Set Font
void oled_set_font(const GFXfont *font);
void oled_print_gfx(uint8_t x, uint8_t y, const char* str);
void oled_draw_char_gfx(uint8_t x, uint8_t y, char c);

#ifdef __cplusplus
}
#endif

#endif