typedef enum
{

  OTA_CODE,
  OTA_INFOR,
  OTA_DATA,
  OTA_RESPONSE
} a;

typedef enum 
{
  OTA_START_CODE,
  OTA_END_CODE,
  OTA_REQUEST_CODE
}Ota_Code_Name_Typdef;  // enum chưa các thành phần của OTA_CODE

typedef enum 
{
  ACK_RESPONSE,
  NACK_RESPONSE
}Ota_Response_Name_Typdef; // enum chưa các thành phần của OTA_Respond

typedef struct __attribute__((packed))
{
  uint8_t command_id; // cho biết đây là tập lệnh
  uint8_t len; // size
  uint8_t ota_code;// cho biết tập lệnh này là tập lệnh gì 
} OTA_Code_t;// Struct chưa các thành phần của OTA_CODE

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t ack; // cho biet day la ACK gi
} OTA_Response_t; // Struct chưa các thành phần của OTA_Respond

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t name[50]; // cho biet ten cua phien ban code
  uint8_t version[10];// cho biet day la phien ban code nao 
} OTA_Infor_t;// Struct chưa các thành phần của OTA_Infor

typedef struct __attribute__((packed))
{
  uint8_t command_id;
  uint8_t len;
  uint8_t data[16]; // data gui di
} OTA_Data_t; // Struct chưa các thành phần của OTA_Data