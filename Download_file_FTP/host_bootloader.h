extern "C"
{
  #include "min.h"
}

#include "Dowload_file.h"
#include "bootloader_cmd.h"
#include "untils.h"
#define MIN_PORT 0
#define MIN_ID 1
#define MIN_Serial Serial2
struct min_context min_ctx;
uint16_t index_file_hex;

typedef enum 
{
  OTA_IDLE_STATE,
  OTA_START_STATE,
  OTA_SEND_INFOR_STATE,
  OTA_SEND_DATA_STATE,
  OTA_END_STATE,
}OTA_State_Typedef;

OTA_State_Typedef ota_state;

void bootloader_send_data (void *data, uint8_t len )
{
  min_send_frame(&min_ctx, MIN_ID,(uint8_t *) data, len);
}
void ota_send_code(Ota_Code_Name_Typdef Code_Name)
{
   OTA_Code_t cmd;
  cmd.command_id = OTA_CODE;
  cmd.len        = 1;
  cmd.ota_code    = Code_Name;
  bootloader_send_data(&cmd, sizeof(cmd));
}

void ota_send_response(Ota_Response_Name_Typdef Response_Name)
{
  OTA_Response_t response;
  response.command_id = OTA_RESPONSE;
  response.len        = 1;
  response.ack        = Response_Name;
  bootloader_send_data( &response, sizeof(response));
}

void ota_send_infor()
{
  OTA_Infor_t infor;
  infor.command_id = OTA_INFOR;
  infor.len        = sizeof(infor.name) + sizeof(infor.version);
  strcpy((char*)&infor.name,"UPDATE OTA");
  strcpy((char*)&infor.version,"1.1");
  bootloader_send_data(&infor, sizeof(infor));
}

extern char file_hex[];
void ota_send_data(uint8_t *data, uint8_t len)
{
  OTA_Data_t ota_data;
  ota_data.command_id = OTA_DATA;
  ota_data.len        = len;
  memcpy(&ota_data.data, data, len);
  bootloader_send_data(&ota_data, sizeof(ota_data));
}


void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port)
{ 
  uint8_t hex_data [21];
  switch (ota_state)
  {
    case OTA_IDLE_STATE:
    {
      OTA_Code_t * ota_code_p = ( OTA_Code_t *) min_payload;
      if (ota_code_p -> command_id == OTA_CODE && ota_code_p -> ota_code == OTA_REQUEST_CODE)
      {
        ota_state = OTA_START_STATE;
        ota_send_code(OTA_START_CODE);
      }
    }
      break;

    case OTA_START_STATE:
    {
      OTA_Response_t * ota_response = (OTA_Response_t*)min_payload;
      if (ota_response-> command_id == OTA_RESPONSE 
        && ota_response->ack == ACK_RESPONSE)
      {
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
    { // gui di dong hex dau tin
      OTA_Response_t * ota_response = (OTA_Response_t*)min_payload;
      if (ota_response-> command_id == OTA_RESPONSE 
        && ota_response->ack == ACK_RESPONSE)
      {
        ota_state = OTA_SEND_DATA_STATE;
        char *token = strtok(file_hex, "\n");
        convert_string_intel_hex_to_array_hex(token, hex_data);
        if (hex_data[3] == 0x00)
          {
            if (check_some(hex_data, hex_data[0] +5) == Check_some_error)
            {
              ota_state = OTA_END_STATE; // neu nhu check some sai la ket thuc luon
              return;
            }
            swap_4_byte(&hex_data[4], hex_data[0]);
            ota_send_data(&hex_data[4], hex_data[0]);
          }
      }
      else 
      {
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;

    case OTA_SEND_DATA_STATE:
    { // gui di cac dong hex con lai
      OTA_Response_t * ota_response = (OTA_Response_t*)min_payload;
      if (ota_response-> command_id == OTA_RESPONSE
        && ota_response->ack == ACK_RESPONSE)
      {
        char *token = strtok(NULL, "\r\n");
        if (token != NULL)
        {
          convert_string_intel_hex_to_array_hex(token, hex_data);
          if (hex_data[3] == 0x00)
          {
            if (check_some(hex_data, hex_data[0] +5) == Check_some_error) // neu nhu check some sai la ket thuc luon
            {
              ota_state = OTA_END_STATE;
              return;
            }
            swap_4_byte(&hex_data[4], hex_data[0]);
            ota_send_data(&hex_data[4], hex_data[0]);
          }
          else
          {
            ota_send_code(OTA_END_CODE);
            ota_state = OTA_END_STATE;
          }
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
      OTA_Response_t * ota_response = (OTA_Response_t*)min_payload;
      if (ota_response-> command_id == OTA_RESPONSE
        && ota_response->ack == ACK_RESPONSE)
      {
        // do something
        ota_state = OTA_IDLE_STATE;
      }
    }
      break;
    default:
      break;
  }
}

void host_bootloader_handle ()
{
  uint8_t c;
  uint8_t len =0;
  if (MIN_Serial.available() > 0)
  {
    c = MIN_Serial.read();
		len = 1;	
  }
  min_poll (&min_ctx, &c, len);
}
void host_bootloader_init()
{
  min_init_context(&min_ctx, MIN_PORT);
  MIN_Serial.begin(115200);
  pinMode(2,OUTPUT);
  ota_state = OTA_IDLE_STATE;
}

// haha 