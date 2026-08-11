#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32f1xx_hal.h"
#include "Uart.h"
#include "min.h"
#include "bootloader_command.h"
#include "Flash.h"

#define FLASH_APP_START_ADDR  0x08004000  // Ð?a ch? n?p App (Trang 16)
#define FLASH_OTA_FLAG_ADDR   0x0800FC00  // Ð?a ch? luu c? tr?ng thái (Trang 63)
#define OTA_FLAG_VALID_MAGIC  0x55AA55AA  

void ota_send_code(Ota_Code_Name_Typdef Code_Name);
void ota_send_response(Ota_Response_Name_Typdef Response_Name);
void bootloader_handle(void);
void bootloader_init(void);
void bootloader_jump_to_app(void); // Hàm chuy?n lu?ng sang App

#endif