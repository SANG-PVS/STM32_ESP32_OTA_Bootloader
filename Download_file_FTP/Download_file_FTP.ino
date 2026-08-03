#include "host_bootloader.h"
#include "port_min.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Khởi tạo & Kiểm tra Version lần đầu khi cấp điện
  dowload_file_init();

  // 2. Khởi tạo MIN protocol và Bootloader
  host_bootloader_init();
}

void loop() {
  // 3. Kiểm tra version.txt trên Cloud định kỳ
  dowload_file_handle();

  // 4. Lắng nghe và xử lý luồng truyền dữ liệu OTA với STM32
  host_bootloader_handle();
}