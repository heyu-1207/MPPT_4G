#include "ota_update.h"
#include "bsp_flash.h"
#include <stdio.h>

static uint32_t s_ota_fw_size = 0;
static uint32_t s_ota_current_offset = 0;

void OTA_Init(void)
{
    /* 使能 CRC 外设时钟，用于校验计算 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
}

/**
  * @brief  开始OTA流程，擦除下载区
  */
void OTA_Begin(uint32_t total_size)
{
    if (total_size > OTA_MAX_FIRMWARE_SIZE) {
        printf("[OTA] Error: Firmware too large!\r\n");
        return;
    }
    s_ota_fw_size = total_size;
    s_ota_current_offset = 0;
    
    printf("[OTA] Erasing Download Area...\r\n");
    BSP_Flash_ErasePages(OTA_DOWNLOAD_ADDR, OTA_MAX_FIRMWARE_SIZE);
    printf("[OTA] Erase Done. Ready to receive %u bytes.\r\n", total_size);
}

/**
  * @brief  写入固件块到下载区
  */
bool OTA_WriteChunk(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (offset + len > OTA_MAX_FIRMWARE_SIZE) {
        return false;
    }
    
    if (BSP_Flash_Write(OTA_DOWNLOAD_ADDR + offset, data, len) != 0) {
        printf("[OTA] Write Chunk Failed at offset %u\r\n", offset);
        return false;
    }
    
    s_ota_current_offset = offset + len;
    return true;
}

/**
  * @brief  完成固件下载，计算CRC并写入标志位，最后重启
  */
bool OTA_FinishAndReboot(void)
{
    if (s_ota_current_offset != s_ota_fw_size || s_ota_fw_size == 0) {
        printf("[OTA] Error: Incomplete firmware.\r\n");
        return false;
    }
    
    /* 1. 计算暂存区整个固件的 CRC32 */
    CRC_ResetDR();
    uint32_t *pData = (uint32_t *)OTA_DOWNLOAD_ADDR;
    uint32_t word_count = s_ota_fw_size / 4;
    for (uint32_t i = 0; i < word_count; i++) {
        CRC_CalcCRC(pData[i]);
    }
    /* 如果结尾有多余的字节没有满4字节，可以用余数补0计算，为简化通常要求固件4字节对齐 */
    uint32_t fw_crc = CRC_GetCRC();
    printf("[OTA] Firmware CRC32 = 0x%08X\r\n", fw_crc);
    
    /* 2. 擦除 OTA 参数区并写入 */
    OTA_Param_t param;
    param.magic = OTA_FLAG_MAGIC;
    param.fw_size = s_ota_fw_size;
    param.fw_crc32 = fw_crc;
    
    printf("[OTA] Writing OTA Flag...\r\n");
    BSP_Flash_ErasePages(OTA_FLAG_ADDR, FLASH_PAGE_SIZE);
    if (BSP_Flash_Write(OTA_FLAG_ADDR, (uint8_t *)&param, sizeof(OTA_Param_t)) != 0) {
        printf("[OTA] Error: Failed to write OTA flag.\r\n");
        return false;
    }
    
    printf("[OTA] Upgrade Ready. Rebooting now...\r\n");
    
    /* 3. 软复位，交由 Bootloader 处理 */
    NVIC_SystemReset();
    
    return true;
}
