#ifndef BOOTLOADER_COMMAND_H
#define BOOTLOADER_COMMAND_H

#include <stdint.h>
typedef enum
{

  OTA_CODE,
  OTA_INFOR,
  OTA_DATA,
  OTA_RESPONSE
} a;   // Enum giai doan chinh trong qua trinh update OTA

typedef enum 
{
  OTA_START_CODE,
  OTA_END_CODE,
  OTA_REQUEST_CODE
}Ota_Code_Name_Typdef;  // Enum Ota_Code cho biet các lenh giao tiep giua stm32 và esp trong qua trinh update OTA

typedef enum 
{
  ACK_RESPONSE,
  NACK_RESPONSE
}Ota_Response_Name_Typdef;  // Enum OTA_Response cho biet cac loai Response Stm32 gui cho Esp32

typedef struct __attribute__((packed))
{
  uint8_t command_id; // cho biết đây là tập lệnh
  uint8_t len; // size
  uint8_t ota_code;// cho biết tập lệnh này là tập lệnh gì 
} OTA_Code_t;  // struct OTA code cho biet mot Frame OTA_Code bao gom nhung gi

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t ack; // cho biet day la ACK gi
} Ota_Response_t; // struct OTA Response cho biet mot Frame OTA_Response bao gom nhung gi

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t name[50]; // cho biet ten cua phien ban code
  uint8_t version[10];// cho biet day la phien ban code nao 
} OTA_Infor_t; // struct OTA Infor cho biet mot Frame OTA_Infor bao gom nhung gi

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t data[16]; // data gui di
} OTA_Data_t; // struct OTA  Data cho biet mot Frame OTA_Data bao gom nhung gi


#endif