#include "Flash.h"

void Flash_unlock()
{
	HAL_FLASH_Unlock();
}

void Flash_lock()
{
	HAL_FLASH_Lock();
	
}


void Flash_erease(uint32_t address)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    //EraseInitStruct.Banks =1;
    EraseInitStruct.PageAddress = address;
    EraseInitStruct.NbPages = 1;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
		uint32_t PageError;
    // khoi tao Flash
    
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

	
}

void Flash_write_arr(uint32_t address, uint8_t *data, uint16_t len)
{
		for (uint16_t i =0; i<len; i+=2){
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + i, data[i]|((uint16_t)data[i+1]<<8));
		}	
}

void Flash_read_arr(uint32_t address, uint8_t *data, uint16_t len)
{
    for (uint16_t i=0; i<len; i+=2){
      uint16_t data_temp = *(volatile uint32_t *)(address + i);
      data[i] = data_temp ;
      data[i+1] = data_temp >> 8;
    }
}