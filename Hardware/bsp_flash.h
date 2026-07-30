#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "stm32f10x.h"

/* STM32F103C8T6 每页大小为 1KB */
#define FLASH_PAGE_SIZE    ((uint16_t)0x400)

void BSP_Flash_ErasePages(uint32_t startAddr, uint32_t size);
uint8_t BSP_Flash_Write(uint32_t startAddr, const uint8_t *pData, uint32_t length);

#endif
