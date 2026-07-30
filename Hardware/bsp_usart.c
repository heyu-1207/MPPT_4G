#include "bsp_usart.h"

/* 取消ARM的半主机工作模式，防止 printf 导致 MCU 卡死 */
#if 1
#pragma import(__use_no_semihosting)
struct __FILE 
{ 
	int handle; 
}; 
FILE __stdout;
void _sys_exit(int x) 
{ 
	x = x; 
}
#endif

/* EC200M 接收缓冲区 */
uint8_t g_ec200m_rx_buf[EC200M_RX_BUF_SIZE];
volatile uint16_t g_ec200m_rx_len = 0;
volatile uint8_t  g_ec200m_rx_flag = 0; // 收到一帧完整数据标志

/* 将 printf 重定向到 UART3 (调试串口) */
int fputc(int ch, FILE *f)
{
    USART_SendData(DEBUG_UART, (uint8_t) ch);
    while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TXE) == RESET);
    return ch;
}

static void EC200M_UART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    /* 1. 时钟使能 */
    EC200M_UART_GPIO_APBxClkCmd(EC200M_UART_GPIO_CLK, ENABLE);
    EC200M_UART_APBxClkCmd(EC200M_UART_CLK, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* 2. GPIO 宏配置 */
    /* TX - 推挽复用 */
    GPIO_InitStructure.GPIO_Pin = EC200M_UART_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EC200M_UART_TX_PORT, &GPIO_InitStructure);
    
    /* RX - 浮空输入或上拉输入 */
    GPIO_InitStructure.GPIO_Pin = EC200M_UART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(EC200M_UART_RX_PORT, &GPIO_InitStructure);

    /* 3. USART 配置 */
    USART_InitStructure.USART_BaudRate = EC200M_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(EC200M_UART, &USART_InitStructure);

    /* 4. DMA 配置 (USART1_RX 使用 DMA1_Channel5) */
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&EC200M_UART->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)g_ec200m_rx_buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = EC200M_RX_BUF_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    DMA_Cmd(DMA1_Channel5, ENABLE);
    USART_DMACmd(EC200M_UART, USART_DMAReq_Rx, ENABLE);

    /* 5. NVIC 与 空闲中断 (IDLE) 配置 */
    NVIC_InitStructure.NVIC_IRQChannel = EC200M_UART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(EC200M_UART, USART_IT_IDLE, ENABLE); // 开启空闲中断
    USART_Cmd(EC200M_UART, ENABLE);
}

static void DEBUG_UART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    DEBUG_UART_GPIO_APBxClkCmd(DEBUG_UART_GPIO_CLK, ENABLE);
    DEBUG_UART_APBxClkCmd(DEBUG_UART_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = DEBUG_UART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = DEBUG_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(DEBUG_UART, &USART_InitStructure);

    USART_Cmd(DEBUG_UART, ENABLE);
}

void BSP_USART_Init(void)
{
    DEBUG_UART_Config();
    EC200M_UART_Config();
    printf("USART Init OK. System Core Clock = %lu\r\n", SystemCoreClock);
}

/* 串口发送字符串到 EC200M */
void EC200M_SendString(const char *str)
{
    while(*str)
    {
        USART_SendData(EC200M_UART, (uint8_t)*str++);
        while (USART_GetFlagStatus(EC200M_UART, USART_FLAG_TXE) == RESET);
    }
}

/* 串口发送指定长度数据到 EC200M (适用于透传) */
void EC200M_SendData(const uint8_t *data, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        USART_SendData(EC200M_UART, data[i]);
        while (USART_GetFlagStatus(EC200M_UART, USART_FLAG_TXE) == RESET);
    }
}

/* 串口1空闲中断处理函数 (需要放到 stm32f10x_it.c 中调用) */
void UART1_IDLE_IRQHandler(void)
{
    uint32_t temp;
    if(USART_GetITStatus(EC200M_UART, USART_IT_IDLE) != RESET)
    {
        temp = EC200M_UART->SR; // 读SR
        temp = EC200M_UART->DR; // 读DR，清IDLE标志
        (void)temp;
        
        DMA_Cmd(DMA1_Channel5, DISABLE); // 关闭DMA，处理数据
        g_ec200m_rx_len = EC200M_RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
        if (g_ec200m_rx_len < EC200M_RX_BUF_SIZE) {
            g_ec200m_rx_buf[g_ec200m_rx_len] = '\0'; // 加上结束符，方便字符串处理
        }
        g_ec200m_rx_flag = 1; // 标记收到完整一帧
        
        // 重新开启DMA
        DMA_SetCurrDataCounter(DMA1_Channel5, EC200M_RX_BUF_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
    }
}
