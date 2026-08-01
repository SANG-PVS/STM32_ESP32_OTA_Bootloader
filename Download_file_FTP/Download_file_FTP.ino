#include "host_bootloader.h"
#include "port_min.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Tải file hex từ FTP Server về mảng file_hex
  dowload_file_init();

  // Khởi tạo MIN protocol và Bootloader
  host_bootloader_init();
}

void loop() {
  // Lắng nghe và xử lý luồng truyền dữ liệu OTA với STM32
  host_bootloader_handle();
}