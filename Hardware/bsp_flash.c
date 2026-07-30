#include "bsp_flash.h"

/**
  * @brief  擦除指定大小的Flash区域
  * @param  startAddr: 起始地址 (必须是页对齐)
  * @param  size: 要擦除的总字节数
  */
void BSP_Flash_ErasePages(uint32_t startAddr, uint32_t size)
{
    uint32_t endAddr = startAddr + size;
    uint32_t currentAddr = startAddr;
    
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    
    while (currentAddr < endAddr) {
        FLASH_ErasePage(currentAddr);
        currentAddr += FLASH_PAGE_SIZE;
    }
    
    FLASH_Lock();
}

/**
  * @brief  向Flash写入数据
  * @param  startAddr: 目标起始地址 (要求半字对齐)
  * @param  pData: 数据源指针
  * @param  length: 写入字节数
  * @retval 0: 成功, 1: 失败
  */
uint8_t BSP_Flash_Write(uint32_t startAddr, const uint8_t *pData, uint32_t length)
{
    uint32_t i;
    uint16_t halfWord;
    
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    
    for (i = 0; i < length; i += 2) {
        /* 如果长度是奇数，最后一个字节补0xFF */
        if ((i + 1) < length) {
            halfWord = (uint16_t)pData[i] | ((uint16_t)pData[i+1] << 8);
        } else {
            halfWord = (uint16_t)pData[i] | 0xFF00;
        }
        
        if (FLASH_ProgramHalfWord(startAddr + i, halfWord) != FLASH_COMPLETE) {
            FLASH_Lock();
            return 1;
        }
    }
    
    FLASH_Lock();
    return 0;
}
