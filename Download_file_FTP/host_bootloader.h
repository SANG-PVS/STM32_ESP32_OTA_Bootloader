#ifndef HOST_BOOTLOADER_H
#define HOST_BOOTLOADER_H

#include <HardwareSerial.h>

extern "C"
{
  #include "min.h"
}

#include "Download_file.h"
#include "bootloader_cmd.h"
#include "untils.h"

// Khởi tạo UART1 phần cứng với chân định nghĩa thủ công
#define RX_PIN 33
#define TX_PIN 32

HardwareSerial UART1(1);

#define MIN_PORT 0
#define MIN_ID 1
#define MIN_Serial UART1

struct min_context min_ctx;

typedef enum 
{
  OTA_IDLE_STATE,
  OTA_START_STATE,
  OTA_SEND_INFOR_STATE,
  OTA_SEND_DATA_STATE,
  OTA_END_STATE,
} OTA_State_Typedef;

OTA_State_Typedef ota_state;

// ================= BỘ HÀM TRUYỀN DỮ LIỆU MIN =================

void bootloader_send_data(void *data, uint8_t len)
{
  min_send_frame(&min_ctx, MIN_ID, (uint8_t *)data, len);
}

void ota_send_code(Ota_Code_Name_Typdef Code_Name)
{
  OTA_Code_t cmd;
  cmd.command_id = OTA_CODE;
  cmd.len        = 1;
  cmd.ota_code   = Code_Name;
  bootloader_send_data(&cmd, sizeof(cmd));
}

void ota_send_response(Ota_Response_Name_Typdef Response_Name)
{
  OTA_Response_t response;
  response.command_id = OTA_RESPONSE;
  response.len        = 1;
  response.ack        = Response_Name;
  bootloader_send_data(&response, sizeof(response));
}

void ota_send_infor()
{
  OTA_Infor_t infor;
  infor.command_id = OTA_INFOR;
  infor.len        = sizeof(infor.name) + sizeof(infor.version);
  
  strcpy((char*)&infor.name, "UPDATE OTA");
  strncpy((char*)&infor.version, new_version.c_str(), sizeof(infor.version) - 1);
  
  bootloader_send_data(&infor, sizeof(infor));
}

void ota_send_data(uint8_t *data, uint8_t len)
{
  OTA_Data_t ota_data;
  ota_data.command_id = OTA_DATA;
  ota_data.len        = len;
  memcpy(&ota_data.data, data, len);
  bootloader_send_data(&ota_data, sizeof(ota_data));
}

// ================= BÓC TÁCH DÒNG INTEL HEX CHUẨN =================

uint8_t get_next_valid_hex_data(uint8_t *hex_data_out)
{
  char *token = NULL;

  while (true)
  {
    if (ota_state == OTA_SEND_INFOR_STATE)
    {
      token = strtok(file_hex, "\r\n");
    }
    else
    {
      token = strtok(NULL, "\r\n");
    }

    if (token == NULL) return END_FILE; // Hết file

    convert_string_intel_hex_to_array_hex(token, hex_data_out);

    // Record Type 0x00: Data Record
    if (hex_data_out[3] == 0x00)
    {
      if (check_some(hex_data_out, hex_data_out[0] + 5) == Check_some_ok)
      {
        swap_4_byte(&hex_data_out[4], hex_data_out[0]);
        return DATA_RECORD;
      }
      else
      {
        Serial.println("[HEX Parser] Checksum error!");
        return Check_some_error;
      }
    }
    // Record Type 0x01: End of File
    else if (hex_data_out[3] == 0x01)
    {
      return END_FILE;
    }
  }
}

// ================= MIN APPLICATION HANDLER =================

void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port)
{ 
  uint8_t hex_data[21];

  switch (ota_state)
  {
    case OTA_IDLE_STATE:
    {
      OTA_Code_t *ota_code_p = (OTA_Code_t *)min_payload;
      if (ota_code_p->command_id == OTA_CODE && ota_code_p->ota_code == OTA_REQUEST_CODE)
      {
        Serial.println("[OTA State] Received OTA_REQUEST_CODE from STM32.");
        ota_state = OTA_START_STATE;
        ota_send_code(OTA_START_CODE);
      }
    }
      break;

    case OTA_START_STATE:
    {
      OTA_Response_t *ota_response = (OTA_Response_t*)min_payload;
      if (ota_response->command_id == OTA_RESPONSE && ota_response->ack == ACK_RESPONSE)
      {
        Serial.println("[OTA State] Handshake OK. Sending OTA_INFOR...");
        ota_state = OTA_SEND_INFOR_STATE;
        ota_send_infor();
      }
      else 
      {
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;

    case OTA_SEND_INFOR_STATE:
    { 
      OTA_Response_t *ota_response = (OTA_Response_t*)min_payload;
      if (ota_response->command_id == OTA_RESPONSE && ota_response->ack == ACK_RESPONSE)
      {
        Serial.println("[OTA State] STM32 ready. Starting hex stream...");
        ota_state = OTA_SEND_DATA_STATE;

        if (get_next_valid_hex_data(hex_data)== DATA_RECORD)
        {
          ota_send_data(&hex_data[4], hex_data[0]);
        }
        else
        {
          Serial.println("[OTA State] No valid hex data found. Sending OTA_END_CODE...");
          ota_send_code(OTA_END_CODE);
          ota_state = OTA_END_STATE;
        }
      }
      else 
      {
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;

    case OTA_SEND_DATA_STATE:
    { 
      OTA_Response_t *ota_response = (OTA_Response_t*)min_payload;
      if (ota_response->command_id == OTA_RESPONSE && ota_response->ack == ACK_RESPONSE)
      {
        if (get_next_valid_hex_data(hex_data) == DATA_RECORD)
        {
          Serial.println("[OTA State] Sending next hex data...");
          ota_send_data(&hex_data[4], hex_data[0]);
        }
        else
        {
          Serial.println("[OTA State] End of hex file reached. Sending OTA_END_CODE...");
          ota_send_code(OTA_END_CODE);
          ota_state = OTA_END_STATE;
        }
      }
      else 
      {
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;

    case OTA_END_STATE:
    {
      OTA_Response_t *ota_response = (OTA_Response_t*)min_payload;
      if (ota_response->command_id == OTA_RESPONSE && ota_response->ack == ACK_RESPONSE)
      {
        Serial.println("[OTA State] Flashing Completed successfully!");
        save_installed_version();
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;

    default:
      break;
  }
}

void host_bootloader_handle()
{
  uint8_t c;
  uint8_t len = 0;
  if (MIN_Serial.available() > 0)
  {
    c = MIN_Serial.read();
    len = 1;	
  }
  min_poll(&min_ctx, &c, len);
}

void host_bootloader_init()
{
  min_init_context(&min_ctx, MIN_PORT);
  
  // Khởi tạo UART1 cứng với RX_PIN = 33, TX_PIN = 32
  MIN_Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(2, OUTPUT);
  ota_state = OTA_IDLE_STATE;
}

#endif // HOST_BOOTLOADER_H