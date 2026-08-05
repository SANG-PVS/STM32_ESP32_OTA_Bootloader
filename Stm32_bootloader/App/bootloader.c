#include "bootloader.h"

#define MIN_PORT 0
#define MIN_ID 1
#define ADDR_APP_PROGRAM 0x08005000U // Địa chỉ bắt đầu phân vùng App

struct min_context min_ctx;

typedef void (*run_app_handler)(void);
run_app_handler run_app;

typedef enum 
{
	OTA_IDLE_STATE,
	OTA_START_STATE,
	OTA_SEND_INFOR_STATE,
	OTA_SEND_DATA_STATE,
	OTA_END_STATE,
} OTA_State_Typedef;

OTA_State_Typedef ota_state;
static uint32_t flash_write_address = ADDR_APP_PROGRAM;

// ================= BỘ HÀM QUẢN LÝ CỜ BKP =================

void set_bkp_flag(uint8_t dr_num, uint16_t val)
{
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    PWR->CR |= PWR_CR_DBP;
    
    if (dr_num == 1)
    {
        BKP->DR1 = val;
    }
    else if (dr_num == 2)
    {
        BKP->DR2 = val;
    }
}

uint16_t get_bkp_flag(uint8_t dr_num)
{
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    
    if (dr_num == 1)
    {
        return (uint16_t)BKP->DR1;
    }
    else if (dr_num == 2)
    {
        return (uint16_t)BKP->DR2;
    }
    return 0;
}

// ================= BỘ HÀM NHẢY APP & XÓA FLASH =================

void run_app_program(void)
{
    HAL_RCC_DeInit();
    HAL_DeInit(); 
    SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk |
                    SCB_SHCSR_BUSFAULTENA_Msk |
                    SCB_SHCSR_MEMFAULTENA_Msk);
    __set_MSP(*((volatile uint32_t*) ADDR_APP_PROGRAM));
    run_app = (run_app_handler)*((volatile uint32_t*) (ADDR_APP_PROGRAM + 4));
    run_app();
}

static void erase_application_zone(void)
{
    Flash_unlock();
    for (uint32_t addr = ADDR_APP_PROGRAM; addr < 0x08010000U; addr += 1024)
    {
        Flash_erease(addr);
    }
    Flash_lock();
}

// ================= TRUYỀN NHẬN MIN PROTOCOL =================

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
				set_bkp_flag(2, 0x0000);
				erase_application_zone();
				flash_write_address = ADDR_APP_PROGRAM;

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
				
				Flash_unlock();
				Flash_write_arr(flash_write_address, ota_data->data, ota_data->len);
				Flash_lock();
				
				flash_write_address += ota_data->len;
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
				Flash_write_arr(flash_write_address, ota_data->data, ota_data->len);
				Flash_lock();
				
				flash_write_address += ota_data->len;
				ota_send_response(ACK_RESPONSE);
			}
			else if (ota_data->command_id == OTA_CODE)
			{
				OTA_Code_t *ota_code = (OTA_Code_t *)min_payload;
				if (ota_code->ota_code == OTA_END_CODE)
				{
					set_bkp_flag(2, 0x1234);
					set_bkp_flag(1, 0x0000);

					ota_state = OTA_END_STATE;
					ota_send_response(ACK_RESPONSE);
					HAL_Delay(100);

					run_app_program();
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

	// PHÁT LẠI REQUEST ĐỊNH KỲ MỖI 300MS CHO ĐẾN KHI ESP32 TRẢ LỜI
	static uint32_t t_retry = 0;
	if (ota_state == OTA_IDLE_STATE)
	{
		if (HAL_GetTick() - t_retry >= 300)
		{
			t_retry = HAL_GetTick();
			bootloader_request_update();
		}
	}
}

void bootloader_init(void)
{
	min_init_context(&min_ctx, MIN_PORT);
	ota_state = OTA_IDLE_STATE;

	uint16_t trigger_flag   = get_bkp_flag(1); // BKP DR1
	uint16_t valid_app_flag = get_bkp_flag(2); // BKP DR2

	uint32_t app_stack = *((volatile uint32_t*) ADDR_APP_PROGRAM);

	// CHỈ JUMP APP KHI MẸO CHECK CON TRỎ STACK NẰM TRONG VÙNG SRAM (0x20000000)
	if (trigger_flag != 0x5A5A && valid_app_flag == 0x1234 && (app_stack & 0x2FFE0000U) == 0x20000000U)
	{
		run_app_program();
	}

	// Xóa cờ lỗi ban đầu nếu chưa có App chuẩn
	set_bkp_flag(2, 0x0000);

	bootloader_request_update();
}