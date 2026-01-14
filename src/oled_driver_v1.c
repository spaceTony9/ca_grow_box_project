// #include "driver/i2c.h"
// #include <string.h>
// #include "oled_driver_v1.h"

// #define I2C_MASTER_SCL_IO 21    /*!< gpio number for I2C master clock */
// #define I2C_MASTER_SDA_IO 19    /*!< gpio number for I2C master */
// #define OLED_I2C_ADDRESS 0x3C  /*!< slave address for OLED display */

// uint8_t buffer[1024]; // Buffer for OLED data

// // Initialize I2C master interface
// void i2c_init() {
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = 400000
//     };
//     i2c_param_config(I2C_NUM_0, &conf);
//     i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
// }

// // Send command to OLED display
// void send_command(uint8_t cmd) {
//     i2c_cmd_handle_t link = i2c_cmd_link_create();
//     i2c_master_start(link);
//     i2c_master_write_byte(link, (OLED_I2C_ADDRESS << 1) | 0, true);
//     i2c_master_write_byte(link, 0x00, true);
//     i2c_master_write_byte(link, cmd, true);
//     i2c_master_stop(link);
//     i2c_master_cmd_begin(I2C_NUM_0, link, pdMS_TO_TICKS(1000));
//     i2c_cmd_link_delete (link);
// }

// // send data to OLED display
// void send_data(uint8_t* data, int len) {
//     i2c_cmd_handle_t link = i2c_cmd_link_create();
//     i2c_master_start(link);
//     i2c_master_write_byte(link, (OLED_I2C_ADDRESS << 1) | 0, true);
//     i2c_master_write_byte(link, 0x40, true);
//     i2c_master_write(link, data, len, true);
//     i2c_master_stop(link);
//     i2c_master_cmd_begin(I2C_NUM_0, link, pdMS_TO_TICKS(1000));
//     i2c_cmd_link_delete (link);
// }

// void oled_init(uint8_t sda_pin, uint8_t scl_pin) {
//     i2c_init();
//     vTaskDelay(pdMS_TO_TICKS(100)); // Time for OLED to power up

//     // Configure OLED
//     send_command(0xAE); // Display off
//     send_command(0xD5); send_command(0x80); // Set display oscillator frequency
//     send_command(0xA8); send_command(0x3F); // Set multiplex ratio
//     send_command(0xD3); send_command(0x00); // Set display offset
//     send_command(0x40);
//     send_command(0x8D); send_command(0x14); // Enable charge pump
//     send_command(0x20); send_command(0x00); // Set memory addressing mode
//     send_command(0xA1); // Set segment re-map
//     send_command(0xC8); // Set COM output scan direction
//     send_command(0xDA); send_command(0x12); // Set COM pins hardware configuration
//     send_command(0x81); send_command(0xFF); // Set contrast control
//     send_command(0xD9); send_command(0xF1); // Set pre-charge period
//     send_command(0xDB); send_command(0x40); // Set VCOMH deselect level
//     send_command(0xA4); // Entire display ON
//     send_command(0xA6); // Set normal display
//     send_command(0xAF); // Display ON
// }

// void oled_clear() {
//     memset(buffer, 0x00, sizeof(buffer));
// }

// void oled_update() {
//     send_command(0x21);// set column address command
//     send_command(0x00);// column start address
//     send_command(0x7F);// column end address
//     send_command(0x22);// set page address command
//     send_command(0x00);// page start address
//     send_command(0x07);// page end address
   
//     for(int i = 0; i < sizeof(buffer); i += 16) {
//         send_data(&buffer[i], 16);
//     }
// }

// void set_pixel(int x, int y, int color) {
//     if(x >= 128 || y >= 64) return;
//     if(color) {
//         buffer[x + (y / 8) * 128] |= (1 << (y % 8));
//     } else {
//         buffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
//     }
// }

// void draw_char(int x, int y, char c) {
//     oled_clear();
//     for(int x = 10; x < 118; x++) {
//         set_pixel(x, 1, 1);
//         set_pixel(x, 50, 1);
//     }
//     for (int y = 10; y < 51; y++) {
//         set_pixel(1, y, 1);
//         set_pixel(117, y, 1);
//     }
//     oled_update();
// }