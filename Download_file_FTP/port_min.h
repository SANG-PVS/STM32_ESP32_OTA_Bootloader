// khai báo các hàm mặc định của MiN

#ifndef PORT_MIN_H
#define PORT_MIN_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "min.h"

#ifdef __cplusplus
}
#endif

void min_tx_start(uint8_t port) {}
 void min_tx_finished(uint8_t port) {}
 uint32_t min_time_ms(void) { return millis(); }
 uint16_t min_tx_space(uint8_t port) { return 512; }

// Dùng trực tiếp Serial2 của ESP32
void min_tx_byte(uint8_t port, uint8_t byte) {
  Serial2.write(byte);
}

#endif