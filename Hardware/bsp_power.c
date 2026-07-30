#include "bsp_power.h"

/**
  * @brief  初始化电源管理和LED的GPIO
  * @param  无
  * @retval 无
  */
void BSP_Power_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启GPIOA和GPIOB时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* ---------------- LED_MCU初始化 (PA6) 及 4G控制引脚 (PA4, PA5) ---------------- */
    GPIO_InitStructure.GPIO_Pin = LED_MCU_PIN | EC200M_RST_PIN | EC200M_DTR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_MCU_PORT, &GPIO_InitStructure);
    
    /* 默认关闭指示灯 (输出高电平) */
    LED_MCU_OFF();
    
    /* 4G复位初始为低电平(不复位)，DTR初始为低电平(唤醒模块) */
    EC200M_RST_LOW();
    EC200M_DTR_LOW();

    /* ---------------- 电源管理引脚初始化 ---------------- */
    /* 24V (PB13), 5V (PB12), 3.8V (PB2) */
    GPIO_InitStructure.GPIO_Pin = POWER_24V_PIN | POWER_5V_PIN | POWER_3V8_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 外部硬件下拉，使能均是高电平。默认输出低电平关闭电源 */
    POWER_24V_OFF();
    POWER_5V_OFF();
    POWER_3V8_OFF();
}
