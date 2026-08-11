#ifndef HOST_BOOTLOADER_H
#define HOST_BOOTLOADER_H

#include <Arduino.h>
#include "port_min.h"
#include "bootloader_cmd.h"
#include "untils.h"
#include "Download_file.h"

#define RX_PIN 16
#define TX_PIN 17

#define MIN_PORT 0
#define MIN_ID 1

struct min_context min_ctx;
static char *hex_ptr = NULL;

typedef enum {
  OTA_IDLE_STATE,
  OTA_START_STATE,
  OTA_SEND_INFOR_STATE,
  OTA_SEND_DATA_STATE,
  OTA_END_STATE,
} OTA_State_Typedef;

OTA_State_Typedef ota_state = OTA_IDLE_STATE;

void clear_uart_rx_buffer(void) {
  while (Serial2.available() > 0) {
    Serial2.read();
  }
}

void bootloader_send_data(void *data, uint8_t len) {
  min_send_frame(&min_ctx, MIN_ID, (uint8_t *)data, len);
}

void ota_send_code(Ota_Code_Name_Typdef Code_Name) {
  OTA_Code_t cmd;
  cmd.command_id = OTA_CODE;
  cmd.len = 1;
  cmd.ota_code = Code_Name;
  bootloader_send_data(&cmd, sizeof(cmd));
}

void ota_send_response(Ota_Response_Name_Typdef Response_Name) {
  OTA_Response_t response;
  response.command_id = OTA_RESPONSE;
  response.len = 1;
  response.ack = Response_Name;
  bootloader_send_data(&response, sizeof(response));
}

void ota_send_infor() {
  OTA_Infor_t infor;
  memset(&infor, 0, sizeof(infor));
  
  infor.command_id = OTA_INFOR;
  infor.len = sizeof(infor.name) + sizeof(infor.version);
  
  strncpy((char*)infor.name, "FTP_FIRMWARE", sizeof(infor.name) - 1);
  strncpy((char*)infor.version, new_version.c_str(), sizeof(infor.version) - 1);
  
  bootloader_send_data(&infor, sizeof(infor));
}

void ota_send_data(uint8_t *data, uint8_t len) {
  OTA_Data_t ota_data;
  ota_data.command_id = OTA_DATA;
  ota_data.len = len;
  memcpy(&ota_data.data, data, len);
  bootloader_send_data(&ota_data, sizeof(ota_data));
}

uint8_t get_next_valid_hex_data(uint8_t *hex_data_out, bool is_first_line) {
  if (is_first_line) {
    hex_ptr = file_hex;
  }

  while (hex_ptr != NULL && *hex_ptr != '\0') {
    char line_buf[128];
    uint8_t idx = 0;

    while (*hex_ptr != '\0' && *hex_ptr != '\r' && *hex_ptr != '\n' && idx < sizeof(line_buf) - 1) {
      line_buf[idx++] = *hex_ptr++;
    }
    line_buf[idx] = '\0';

    while (*hex_ptr == '\r' || *hex_ptr == '\n') {
      hex_ptr++;
    }

    if (idx == 0) continue;

    convert_string_intel_hex_to_array_hex(line_buf, hex_data_out);

    if (hex_data_out[3] == 0x00) { // Record Data
      if (check_some(hex_data_out, hex_data_out[0] + 5) == Check_some_ok) {
        //swap_4_byte(&hex_data_out[4], hex_data_out[0]);
        return 1;
      }
    } else if (hex_data_out[3] == 0x01) { // EOF Record
      return 0;
    }
  }
  return 0;
}

void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port) {
  uint8_t hex_data[21];

  OTA_Code_t *ota_code_p = (OTA_Code_t *)min_payload;
  if (ota_code_p->command_id == OTA_CODE && ota_code_p->ota_code == OTA_REQUEST_CODE) {
    // Trường hợp 1: Có Firmware mới trong RAM -> Bắt đầu nạp OTA
    if (strlen(file_hex) > 0) {
      Serial.println("\n[ESP32] <<< Nhan REQUEST tu STM32 & Co Firmware moi san sang!");
      ota_state = OTA_START_STATE;
      Serial.println("[ESP32] >>> Gui OTA_START_CODE...");
      ota_send_code(OTA_START_CODE);
      return;
    } 
    // Trường hợp 2: Không có Firmware mới (mới nhất rồi) -> Trả lời OTA_END_CODE để STM32 biết
    else {
      static unsigned long last_log = 0;
      if (millis() - last_log >= 3000) {
        last_log = millis();
        Serial.println("[ESP32] <<< Nhan REQUEST tu STM32 nhung Firmware da la moi nhat. Báo STM32 khong co Update!");
      }
      ota_send_code(OTA_END_CODE); // Gửi báo không có bản update mới
      return;
    }
  }

  switch (ota_state) {
    case OTA_START_STATE: {
      OTA_Response_t *res = (OTA_Response_t*)min_payload;
      if (res->command_id == OTA_RESPONSE && res->ack == ACK_RESPONSE) {
        Serial.println("[ESP32] <<< Nhan ACK Start!");
        ota_state = OTA_SEND_INFOR_STATE;
        Serial.println("[ESP32] >>> Gui OTA_INFOR...");
        ota_send_infor();
      }
      break;
    }

    case OTA_SEND_INFOR_STATE: {
      OTA_Response_t *res = (OTA_Response_t*)min_payload;
      if (res->command_id == OTA_RESPONSE && res->ack == ACK_RESPONSE) {
        Serial.println("[ESP32] <<< Nhan ACK Infor!");
        ota_state = OTA_SEND_DATA_STATE;
        
        if (get_next_valid_hex_data(hex_data, true)) {
          Serial.println("[ESP32] >>> Gui goi DATA dau tien...");
          ota_send_data(&hex_data[4], hex_data[0]);
        } else {
          Serial.println("[ESP32] >>> File HEX rong! Gui OTA_END_CODE...");
          ota_send_code(OTA_END_CODE);
          ota_state = OTA_END_STATE;
        }
      }
      break;
    }

    case OTA_SEND_DATA_STATE: {
      OTA_Response_t *res = (OTA_Response_t*)min_payload;
      if (res->command_id == OTA_RESPONSE && res->ack == ACK_RESPONSE) {
        Serial.println("[ESP32] <<< Nhan ACK Data!");
        
        if (get_next_valid_hex_data(hex_data, false)) {
          Serial.println("[ESP32] >>> Gui goi DATA tiep theo...");
          ota_send_data(&hex_data[4], hex_data[0]);
        } else {
          Serial.println("[ESP32] >>> Da truyen het File HEX Cloud! Gui OTA_END_CODE...");
          ota_send_code(OTA_END_CODE);
          ota_state = OTA_END_STATE;
        }
      }
      break;
    }

    case OTA_END_STATE: {
      OTA_Response_t *res = (OTA_Response_t*)min_payload;
      if (res->command_id == OTA_RESPONSE && res->ack == ACK_RESPONSE) {
        Serial.println("\n[ESP32] === THANH CONG! Da truyen xong toan bo File HEX sang STM32 ===");
        save_installed_version();
        memset(file_hex, 0, sizeof(file_hex));
        ota_state = OTA_IDLE_STATE;
      }
      break;
    }
  }
}

void host_bootloader_init() {
  min_init_context(&min_ctx, MIN_PORT);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  ota_state = OTA_IDLE_STATE;
  Serial.println("[ESP32] Host Bootloader & Serial2 Ready!");
}

void host_bootloader_handle() {
  while (Serial2.available() > 0) {
    uint8_t c = Serial2.read();
    min_poll(&min_ctx, &c, 1);
  }
}

#endif