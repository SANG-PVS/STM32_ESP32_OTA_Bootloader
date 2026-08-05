#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
#include "stm32f1xx_hal.h"
void Flash_erease(uint32_t address);
void Flash_write_arr(uint32_t address, uint8_t *data, uint16_t len);
void Flash_read_arr(uint32_t address, uint8_t *data, uint16_t len);
void Flash_unlock();
void Flash_lock();
#endif