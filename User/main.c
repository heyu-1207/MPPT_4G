#include "stm32f10x.h"
#include "bsp_power.h"
#include "bsp_usart.h"
#include "ec200m_api.h"
#include "ota_update.h"
#include <stdio.h>

volatile uint32_t g_sys_tick = 0;

int main(void)
{
    /* 获取硬件真实的运行时钟频率，这块板子没有外部晶振，只能跑在内部 8MHz 上，
       如果不调用这个函数更新 SystemCoreClock，系统滴答定时器就会慢 9 倍！ */
    SystemCoreClockUpdate();

    /* 0. OTA升级要求: 重新映射中断向量表到 APP 起始地址 (0x08003000) */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x3000);
    
    /* 极其重要：Bootloader 跳转前关闭了全局中断(__disable_irq)，必须在这里重新打开！ */
    __enable_irq();

    /* 1. 初始化中断向量和 SysTick (1ms) */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SysTick_Config(SystemCoreClock / 1000);

    /* 2. 初始化底层外设和模块 */
    OTA_Init();
    BSP_Power_Init();
    BSP_USART_Init();
    
    printf("\r\n--- MPPT 4G Module Booting ---\r\n");
    
    /* 3. 开启各路电源 */
    POWER_24V_ON();
    POWER_5V_ON();
    POWER_3V8_ON();
    LED_MCU_ON();
    
    /* 4. 初始化 EC200M 并设置工作模式 (例如选择 MQTT 模式) */
    EC200M_Init();
    
    /* TODO: 您可以在这里切换为 EC200M_MODE_TCP 来进行 TCP 透传测试 */
    EC200M_SetMode(EC200M_MODE_MQTT);
    
    uint32_t last_pub_tick = 0;

    while(1)
    {
        /* 核心非阻塞轮询状态机 */
        EC200M_Poll(g_sys_tick);
        
        /* 示例：每 10 秒发布一次 MQTT 消息或通过 TCP 透传发送数据 */
        if (g_sys_tick - last_pub_tick >= 10000) {
            last_pub_tick = g_sys_tick;
            
            if (EC200M_IsConnected()) {
                printf("[APP] Sending data to server...\r\n");
                
                /* 如果是 MQTT 模式 */
                EC200M_MQTT_Publish("mppt/data", "{\"v\":24.5,\"c\":1.2}");
                
                /* 如果是 TCP 模式，则可以使用下面这句：*/
                // EC200M_TCP_Send((uint8_t*)"{\"v\":24.5,\"c\":1.2}", 19);
            }
        }
    }
}