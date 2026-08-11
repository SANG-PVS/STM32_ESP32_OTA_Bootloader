#include "host_bootloader.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  host_bootloader_init();

  // Tải Firmware FTP từ Cloud về RAM
  dowload_file_init();

  // Xóa sạch bộ đệm UART rác tích tụ trong lúc bận tải Wi-Fi / FTP
  clear_uart_rx_buffer();
  min_init_context(&min_ctx, MIN_PORT); // Reset lại MIN context chuẩn
  Serial.println("[ESP32] Da xoa ruc UART RX. San sang bat tay OTA tu STM32!");
}

void loop() {
  dowload_file_handle();
  host_bootloader_handle();
}