#include "host_bootloader.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Khởi tạo UART giao tiếp với STM32
  host_bootloader_init();

  // Khởi tạo đọc NVS & Kết nối Wi-Fi sẵn sàng
  dowload_file_init();

  // Xóa rác đệm UART RX trước khi sẵn sàng
  clear_uart_rx_buffer();
  min_init_context(&min_ctx, MIN_PORT);

  Serial.println("[ESP32] Ready! Cho tin hieu OTA Request tu STM32...");
}

void loop() {
  // Lắng nghe lệnh từ STM32
  host_bootloader_handle();
}