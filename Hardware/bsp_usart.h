#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

/* EC200M 通信串口 (UART1) 宏定义 */
#define EC200M_UART              USART1
#define EC200M_UART_CLK          RCC_APB2Periph_USART1
#define EC200M_UART_APBxClkCmd   RCC_APB2PeriphClockCmd
#define EC200M_UART_BAUDRATE     115200

#define EC200M_UART_GPIO_CLK     RCC_APB2Periph_GPIOA
#define EC200M_UART_GPIO_APBxClkCmd RCC_APB2PeriphClockCmd
#define EC200M_UART_TX_PORT      GPIOA
#define EC200M_UART_TX_PIN       GPIO_Pin_9
#define EC200M_UART_RX_PORT      GPIOA
#define EC200M_UART_RX_PIN       GPIO_Pin_10
#define EC200M_UART_IRQ          USART1_IRQn

/* 调试输出串口 (UART3) 宏定义 */
#define DEBUG_UART               USART3
#define DEBUG_UART_CLK           RCC_APB1Periph_USART3
#define DEBUG_UART_APBxClkCmd    RCC_APB1PeriphClockCmd
#define DEBUG_UART_BAUDRATE      115200

#define DEBUG_UART_GPIO_CLK      RCC_APB2Periph_GPIOB
#define DEBUG_UART_GPIO_APBxClkCmd RCC_APB2PeriphClockCmd
#define DEBUG_UART_TX_PORT       GPIOB
#define DEBUG_UART_TX_PIN        GPIO_Pin_10
#define DEBUG_UART_RX_PORT       GPIOB
#define DEBUG_UART_RX_PIN        GPIO_Pin_11

/* 环形缓冲区大小 */
#define EC200M_RX_BUF_SIZE       512

extern uint8_t g_ec200m_rx_buf[EC200M_RX_BUF_SIZE];
extern volatile uint16_t g_ec200m_rx_len;
extern volatile uint8_t  g_ec200m_rx_flag;

void BSP_USART_Init(void);
void EC200M_SendString(const char *str);
void EC200M_SendData(const uint8_t *data, uint16_t len);
void UART1_IDLE_IRQHandler(void);

#endif /* __BSP_USART_H */
