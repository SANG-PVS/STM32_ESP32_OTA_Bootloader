#include "bootloader.h"

#define MIN_PORT 0
#define MIN_ID 1

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
static uint32_t t_last_rx = 0; // Biến giám sát Timeout

static void bootloader_send_data(void *data, uint8_t len)
{
	min_send_frame(&min_ctx, MIN_ID, (uint8_t *) data, len);
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
	Ota_Response_t response;
	response.command_id = OTA_RESPONSE;
	response.len        = 1;
	response.ack        = Response_Name;
	bootloader_send_data(&response, sizeof(response));
}

void bootloader_request_update(void)
{
	ota_send_code(OTA_REQUEST_CODE);
}

void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port)
{
	t_last_rx = HAL_GetTick(); // Cập nhật mốc thời gian mỗi khi nhận dữ liệu thành công

	switch (ota_state)
	{
		case OTA_IDLE_STATE:
		{
			OTA_Code_t * ota_code_p = (OTA_Code_t *) min_payload;
			if (ota_code_p->command_id == OTA_CODE 
				&& ota_code_p->ota_code == OTA_START_CODE)
			{
				ota_state = OTA_START_STATE;
				ota_send_response(ACK_RESPONSE);
			}
		}
			break;
		
		case OTA_START_STATE:
		{
			OTA_Infor_t *ota_infor = (OTA_Infor_t*)min_payload;
			if (ota_infor->command_id == OTA_INFOR)
			{
				ota_state = OTA_SEND_INFOR_STATE;
				ota_send_response(ACK_RESPONSE);
			}
			else 
			{
				ota_state = OTA_IDLE_STATE;
			}
		}
			break;
		
		case OTA_SEND_INFOR_STATE:
		{	
			OTA_Data_t *ota_data = (OTA_Data_t*)min_payload;
			if (ota_data->command_id == OTA_DATA)
			{
				ota_state = OTA_SEND_DATA_STATE;
				ota_send_response(ACK_RESPONSE);
			}
			else 
			{
				ota_state = OTA_IDLE_STATE;
			}
		}
			break;

		case OTA_SEND_DATA_STATE:
		{	
			OTA_Data_t *ota_data = (OTA_Data_t*)min_payload;
			if (ota_data->command_id == OTA_DATA)
			{
				ota_send_response(ACK_RESPONSE);
			}
			else if (ota_data->command_id == OTA_CODE)
			{
				OTA_Code_t *ota_code = (OTA_Code_t *)min_payload;
				if (ota_code->ota_code == OTA_END_CODE)
				{
					ota_state = OTA_END_STATE;
					ota_send_response(ACK_RESPONSE);
				}
			}
			else 
			{
				ota_state = OTA_IDLE_STATE;
			}
		}
			break;

		default:
			break;
	}
}

void bootloader_handle(void)
{
	uint8_t data;
	uint8_t len = 0;
	if (Uart_available() > 0)
	{
		data = Uart_read();
		len = 1;	
	}
	
	min_poll(&min_ctx, &data, len);

	// 1. Phát Request định kỳ 300ms khi ở OTA_IDLE_STATE
	static uint32_t t_retry = 0;
	if (ota_state == OTA_IDLE_STATE)
	{
		if (HAL_GetTick() - t_retry >= 300)
		{
			t_retry = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Nháy LED PC13
			bootloader_request_update();
		}
	}
	// 2. CƠ CHẾ TIMEOUT 3 Giây: Nếu đang truyền dở mà mất tín hiệu với ESP32 -> Reset về IDLE để kết nối lại
	else
	{
		if (HAL_GetTick() - t_last_rx >= 3000)
		{
			ota_state = OTA_IDLE_STATE;
		}
	}
}

void bootloader_init(void)
{
	min_init_context(&min_ctx, MIN_PORT);
	ota_state = OTA_IDLE_STATE;
	t_last_rx = HAL_GetTick();
}