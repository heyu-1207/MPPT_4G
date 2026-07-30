#ifndef __OTA_UPDATE_H
#define __OTA_UPDATE_H

#include "stm32f10x.h"
#include <stdbool.h>

/* OTA 相关地址宏定义 (假设总容量 64KB, 见文档规划) */
#define OTA_APP_ADDR           0x08003000   /* 24KB APP 运行区起始地址 */
#define OTA_DOWNLOAD_ADDR      0x08009000   /* 24KB OTA 暂存区起始地址 */
#define OTA_MAX_FIRMWARE_SIZE  0x6000       /* 24KB 最大固件大小 */

#define OTA_FLAG_ADDR          0x0800F000   /* 1KB OTA 标志位存放区 */
#define OTA_FLAG_MAGIC         0x5A5A5A5A   /* 固件就绪待更新魔数 */
#define OTA_FLAG_CLEARED       0xFFFFFFFF   /* 固件已更新或无需更新魔数 */

#pragma pack(push, 4)
typedef struct {
    uint32_t magic;         /* 标志位魔数 */
    uint32_t fw_size;       /* 固件总大小 */
    uint32_t fw_crc32;      /* 固件整包 CRC32 校验和 (STM32 内置 CRC 外设计算) */
} OTA_Param_t;
#pragma pack(pop)

void OTA_Init(void);
void OTA_Begin(uint32_t total_size);
bool OTA_WriteChunk(uint32_t offset, const uint8_t *data, uint32_t len);
bool OTA_FinishAndReboot(void);

#endif
