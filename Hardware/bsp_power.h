#ifndef __BSP_POWER_H
#define __BSP_POWER_H

#include "stm32f10x.h"

/* 电源管理引脚定义 */
#define POWER_24V_PORT      GPIOB
#define POWER_24V_PIN       GPIO_Pin_13

#define POWER_5V_PORT       GPIOB
#define POWER_5V_PIN        GPIO_Pin_12

#define POWER_3V8_PORT      GPIOB
#define POWER_3V8_PIN       GPIO_Pin_2

/* MCU指示灯引脚定义 */
#define LED_MCU_PORT        GPIOA
#define LED_MCU_PIN         GPIO_Pin_6

/* 4G模组控制引脚定义 */
#define EC200M_RST_PORT     GPIOA
#define EC200M_RST_PIN      GPIO_Pin_4
#define EC200M_DTR_PORT     GPIOA
#define EC200M_DTR_PIN      GPIO_Pin_5

/* 宏定义功能函数 */
#define POWER_24V_ON()      GPIO_SetBits(POWER_24V_PORT, POWER_24V_PIN)
#define POWER_24V_OFF()     GPIO_ResetBits(POWER_24V_PORT, POWER_24V_PIN)

#define POWER_5V_ON()       GPIO_SetBits(POWER_5V_PORT, POWER_5V_PIN)
#define POWER_5V_OFF()      GPIO_ResetBits(POWER_5V_PORT, POWER_5V_PIN)

#define POWER_3V8_ON()      GPIO_SetBits(POWER_3V8_PORT, POWER_3V8_PIN)
#define POWER_3V8_OFF()     GPIO_ResetBits(POWER_3V8_PORT, POWER_3V8_PIN)

/* LED阴极接PA6，阳极接电源，低电平点亮 */
#define LED_MCU_ON()        GPIO_ResetBits(LED_MCU_PORT, LED_MCU_PIN)
#define LED_MCU_OFF()       GPIO_SetBits(LED_MCU_PORT, LED_MCU_PIN)
#define LED_MCU_TOGGLE()    (LED_MCU_PORT->ODR ^= LED_MCU_PIN)

/* 4G模组控制: 复位高电平有效，DTR低电平唤醒 */
#define EC200M_RST_HIGH()   GPIO_SetBits(EC200M_RST_PORT, EC200M_RST_PIN)
#define EC200M_RST_LOW()    GPIO_ResetBits(EC200M_RST_PORT, EC200M_RST_PIN)

#define EC200M_DTR_HIGH()   GPIO_SetBits(EC200M_DTR_PORT, EC200M_DTR_PIN)
#define EC200M_DTR_LOW()    GPIO_ResetBits(EC200M_DTR_PORT, EC200M_DTR_PIN)

void BSP_Power_Init(void);

#endif
