#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32f1xx.h"
#include "Uart.h"
#include "min.h"
#include "bootloader_command.h"
void ota_send_code(Ota_Code_Name_Typdef Code_Name);
void ota_send_response(Ota_Response_Name_Typdef Response_Name);
void bootloader_handle();
void bootloader_init();
#endif