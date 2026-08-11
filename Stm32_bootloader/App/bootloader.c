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
static uint32_t t_last_rx = 0;
static uint32_t current_flash_addr = FLASH_APP_START_ADDR;

// Con trỏ hàm dùng để nhảy sang App
typedef void (*pFunction)(void);

void bootloader_jump_to_app(void)
{
	uint32_t jump_addr = *(__IO uint32_t*) (FLASH_APP_START_ADDR + 4);
	pFunction JumpToApplication = (pFunction) jump_addr;

	// Kiểm tra địa chỉ Stack Pointer có hợp lệ (nằm trong vùng RAM) không
	if (((*(__IO uint32_t*)FLASH_APP_START_ADDR) & 0x2FFE0000) == 0x20000000)
	{
		// Tắt tất cả ngoại vi & ngắt trước khi chuyển giao quyền điều khiển
		HAL_RCC_DeInit();
		HAL_DeInit();
		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0;

		// Cài đặt lại con trỏ Main Stack Pointer (MSP) sang địa chỉ của App
		__set_MSP(*(__IO uint32_t*) FLASH_APP_START_ADDR);

		// Nhảy trực tiếp vào hàm main() của App
		JumpToApplication();
	}
}

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

static void bootloader_erase_app_flash(void)
{
	Flash_unlock();
	for (uint32_t addr = FLASH_APP_START_ADDR; addr < FLASH_OTA_FLAG_ADDR; addr += 1024)
	{
		Flash_erease(addr);
	}
	Flash_erease(FLASH_OTA_FLAG_ADDR);
	Flash_lock();
}

void min_application_handler(uint8_t min_id, uint8_t const *min_payload, uint8_t len_payload, uint8_t port)
{
	t_last_rx = HAL_GetTick();

	switch (ota_state)
	{
		case OTA_IDLE_STATE:
		{
			OTA_Code_t * ota_code_p = (OTA_Code_t *) min_payload;
			if (ota_code_p->command_id == OTA_CODE)
			{
				// Trường hợp 1: ESP32 báo CÓ bản cập nhật mới -> Xóa Flash và chuẩn bị nạp
				if (ota_code_p->ota_code == OTA_START_CODE)
				{
					bootloader_erase_app_flash();
					current_flash_addr = FLASH_APP_START_ADDR;

					ota_state = OTA_START_STATE;
					ota_send_response(ACK_RESPONSE);
				}
				// Trường hợp 2: ESP32 báo KHÔNG CÓ bản cập nhật mới
				else if (ota_code_p->ota_code == OTA_END_CODE)
				{
					uint32_t current_ota_flag = *(volatile uint32_t*)FLASH_OTA_FLAG_ADDR;
					// Nếu Flash đang có sẵn App hợp lệ -> Nhảy sang App ngay lập tức
					if (current_ota_flag == OTA_FLAG_VALID_MAGIC)
					{
						bootloader_jump_to_app();
					}
				}
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
				Flash_unlock();
				Flash_write_arr(current_flash_addr, ota_data->data, ota_data->len);
				Flash_lock();
				current_flash_addr += ota_data->len;

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
				Flash_unlock();
				Flash_write_arr(current_flash_addr, ota_data->data, ota_data->len);
				Flash_lock();
				current_flash_addr += ota_data->len;

				ota_send_response(ACK_RESPONSE);
			}
			else if (ota_data->command_id == OTA_CODE)
			{
				OTA_Code_t *ota_code = (OTA_Code_t *)min_payload;
				if (ota_code->ota_code == OTA_END_CODE)
				{
					// Nạp hoàn tất 100% -> Ghi cờ VALID
					uint32_t flag_valid = OTA_FLAG_VALID_MAGIC;
					Flash_unlock();
					Flash_write_arr(FLASH_OTA_FLAG_ADDR, (uint8_t*)&flag_valid, 4);
					Flash_lock();

					ota_state = OTA_END_STATE;
					ota_send_response(ACK_RESPONSE);

					// Cho delay ngắn rồi nhảy sang App
					HAL_Delay(100);
					bootloader_jump_to_app();

					// Phòng hờ nếu lệnh nhảy trả về (App lỗi), giữ treo ở đây không cho chạy trôi xuống dưới
					while(1)
					{
						HAL_Delay(1000);
					}
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

	static uint32_t t_retry = 0;

	// Nếu đang ở IDLE (chưa bắt tay được OTA) -> Lặp lại gửi Request mỗi 300ms
	if (ota_state == OTA_IDLE_STATE)
	{
		if (HAL_GetTick() - t_retry >= 300)
		{
			t_retry = HAL_GetTick();
			bootloader_request_update();
		}
	}
	else
	{
		// Timeout 3 giây bảo vệ nếu mất kết nối trong quá trình nạp
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

	uint32_t current_ota_flag = *(volatile uint32_t*)FLASH_OTA_FLAG_ADDR;
	
	// 1. Nếu đã có App hợp lệ -> Nhảy thẳng vào App NGAY LẬP TỨC
	if (current_ota_flag == OTA_FLAG_VALID_MAGIC)
	{
		bootloader_jump_to_app();
	}
	else
	{
		// 2. Nếu cờ không hợp lệ -> Gửi Request yêu cầu update ngay
		bootloader_request_update();
	}
}