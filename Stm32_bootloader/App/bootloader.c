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
}OTA_State_Typedef;

OTA_State_Typedef ota_state;

static void bootloader_send_data (void *data, uint8_t len )
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
	Ota_Response_t response;
	response.command_id = OTA_RESPONSE;
	response.len        = 1;
	response.ack        = Response_Name;
	bootloader_send_data( &response, sizeof(response));
}

void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port)
{
	switch (ota_state)
	{
		case OTA_IDLE_STATE:
		{
			OTA_Code_t * ota_code_p = ( OTA_Code_t *) min_payload;
			if (ota_code_p -> command_id == OTA_CODE 
				&& ota_code_p -> ota_code == OTA_START_CODE)
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
				// luu ten va version vao flash
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
				// dong dau tien
				// lay data va luu vao vung app
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
				// cac dong con lai
				// lay data va luu vao vung app
				ota_send_response(ACK_RESPONSE);
			}

			else if (ota_data->command_id == OTA_CODE)
			{
				OTA_Code_t *ota_code = (OTA_Code_t *)min_payload;
				if (ota_code->ota_code == OTA_END_CODE)
				{
					ota_state = OTA_END_STATE;
					ota_send_response(ACK_RESPONSE);
					// run_app();
				}
			}
			else 
			{
				ota_state = OTA_IDLE_STATE;
			}
		}
			break;

		// case OTA_END_STATE:
		// 	ota_send_response(ACK_RESPONSE);
		// 	break;
		
		default:
			break;
	}
}

void bootloader_request_update()
{
	uint8_t cmd[] = {0x00,0x01, 0x02};
	min_send_frame( &min_ctx, MIN_ID, cmd, 3);
}

void bootloader_handle()
{
	uint8_t data;
	uint8_t len = 0;
	if (Uart_available() > 0)
		{
			data = Uart_read();
			len = 1;	
		}
	
	min_poll (&min_ctx, &data, len);
}

void bootloader_init()
{
	min_init_context(&min_ctx, MIN_PORT);
	ota_state = OTA_IDLE_STATE;
}