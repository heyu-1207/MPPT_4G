#include "ec200m_api.h"
#include "bsp_usart.h"
#include "bsp_power.h"
#include <stdio.h>
#include <string.h>

#define TCP_SERVER_IP   "114.114.114.114"
#define TCP_SERVER_PORT "8080"

#define MQTT_SERVER_IP  "iot-06z00but8eujbsv.mqtt.iothub.aliyuncs.com"
#define MQTT_SERVER_PORT "1883"
#define MQTT_CLIENT_ID  "imakt5zOK0U.MQTT_TEST1|securemode=2,signmethod=hmacsha256,timestamp=1785307132406|"
#define MQTT_USER       "MQTT_TEST1&imakt5zOK0U"
#define MQTT_PWD        "518403c9901b804bd75936bf93610a238066ff671efe95d2c87266b4161fc87d"

static EC200M_State_t g_modem_state = EC200M_STATE_OFF;
static EC200M_Mode_t  g_work_mode = EC200M_MODE_NONE;
static uint32_t       g_state_timer = 0;
static uint32_t       g_last_tick = 0;
static uint8_t        g_cmd_retry_cnt = 0;

/* 发送 AT 指令并重置超时 */
static void Send_AT_Cmd(const char *cmd)
{
    printf("[AT TX]: %s", cmd); // 调试输出
    g_ec200m_rx_len = 0;
    g_ec200m_rx_flag = 0;
    memset(g_ec200m_rx_buf, 0, EC200M_RX_BUF_SIZE);
    EC200M_SendString(cmd);
}

/* 检查是否收到期望响应 */
static bool Check_Resp(const char *target)
{
    if (g_ec200m_rx_flag) {
        g_ec200m_rx_buf[g_ec200m_rx_len] = '\0'; /* 加上结束符 */
        
        /* 增加调试打印，看看模块到底有没有回乱码 */
        printf("[RAW RX]: %s\r\n", g_ec200m_rx_buf);
        
        if (strstr((const char*)g_ec200m_rx_buf, target) != NULL) {
            printf("[AT RX OK]: %s\n", target);
            g_ec200m_rx_flag = 0; // 消耗掉该帧
            return true;
        }
    }
    return false;
}

/* 拦截断线 URC */
static void Check_URC(void)
{
    if (g_ec200m_rx_flag) {
        if (strstr((const char *)g_ec200m_rx_buf, "+QIURC: \"closed\"") ||
            strstr((const char *)g_ec200m_rx_buf, "+QMTSTAT:")) {
            printf("\r\n[URC] Network disconnected! Reconnecting...\r\n");
            g_ec200m_rx_flag = 0;
            g_modem_state = EC200M_STATE_CHECK_SIM; // 跳回检查SIM卡步骤重连
        }
    }
}

void EC200M_Init(void)
{
    g_modem_state = EC200M_STATE_POWER_ON;
    g_state_timer = 0;
    g_cmd_retry_cnt = 0;
}

void EC200M_SetMode(EC200M_Mode_t mode)
{
    g_work_mode = mode;
}

bool EC200M_IsConnected(void)
{
    return (g_modem_state == EC200M_STATE_TCP_TRANSPARENT) ||
           (g_modem_state == EC200M_STATE_MQTT_IDLE);
}

bool EC200M_TCP_Send(const uint8_t *data, uint16_t len)
{
    if (g_modem_state == EC200M_STATE_TCP_TRANSPARENT) {
        EC200M_SendData(data, len);
        return true;
    }
    return false;
}

bool EC200M_MQTT_Publish(const char *topic, const char *payload)
{
    if (g_modem_state == EC200M_STATE_MQTT_IDLE) {
        char pub_cmd[256];
        snprintf(pub_cmd, sizeof(pub_cmd), "AT+QMTPUB=0,0,0,0,\"%s\",\"%s\"\r\n", topic, payload);
        Send_AT_Cmd(pub_cmd);
        // 这里只是发送，未做非阻塞确认，实际应用中可以将其加入状态机队列
        return true;
    }
    return false;
}

/* 核心非阻塞状态机 (主循环中调用) */
void EC200M_Poll(uint32_t sys_tick)
{
    uint32_t dt = sys_tick - g_last_tick;
    g_last_tick = sys_tick;
    g_state_timer += dt;

    if (g_work_mode == EC200M_MODE_NONE) return;

    Check_URC(); // 每次优先拦截 URC

    switch (g_modem_state) {
        case EC200M_STATE_OFF:
            break;

        case EC200M_STATE_POWER_ON:
            /* 之前系统慢9倍时，一切工作正常。我们现在完美复刻慢9倍时的绝对时序！
               0 ~ 4500ms : 拉高开机脚
               4500 ~ 18000ms: 拉低开机脚 (13.5秒)
               > 27000ms : 发送首条 AT指令 */
            if (g_state_timer < 4500) {
                EC200M_RST_HIGH();
            } else if (g_state_timer >= 4500 && g_state_timer < 18000) {
                EC200M_RST_LOW();
            } else if (g_state_timer >= 27000) {
                Send_AT_Cmd("AT\r\n");
                g_modem_state = EC200M_STATE_INIT_AT;
                g_state_timer = 0;
            }
            break;

        case EC200M_STATE_INIT_AT:
            if (Check_Resp("OK")) {
                Send_AT_Cmd("ATE0\r\n");
                g_modem_state = EC200M_STATE_ECHO_OFF;
                g_state_timer = 0;
            } else if (g_state_timer > 1000) {
                Send_AT_Cmd("AT\r\n");
                g_state_timer = 0;
            }
            break;

        case EC200M_STATE_ECHO_OFF:
            if (Check_Resp("OK")) {
                Send_AT_Cmd("AT+CPIN?\r\n");
                g_modem_state = EC200M_STATE_CHECK_SIM;
                g_state_timer = 0;
            } else if (g_state_timer > 1000) {
                g_modem_state = EC200M_STATE_INIT_AT;
            }
            break;

        case EC200M_STATE_CHECK_SIM:
            if (Check_Resp("+CPIN: READY")) {
                Send_AT_Cmd("AT+CREG?\r\n");
                g_modem_state = EC200M_STATE_CHECK_CREG;
                g_state_timer = 0;
            } else if (g_state_timer > 2000) {
                Send_AT_Cmd("AT+CPIN?\r\n");
                g_state_timer = 0;
            }
            break;

        case EC200M_STATE_CHECK_CREG:
            if (Check_Resp("+CREG: 0,1") || Check_Resp("+CREG: 0,5")) {
                Send_AT_Cmd("AT+CEREG?\r\n");
                g_modem_state = EC200M_STATE_CHECK_CGREG;
                g_state_timer = 0;
            } else if (g_state_timer > 2000) {
                Send_AT_Cmd("AT+CREG?\r\n");
                g_state_timer = 0;
            }
            break;

        case EC200M_STATE_CHECK_CGREG:
            if (Check_Resp("+CEREG: 0,1") || Check_Resp("+CEREG: 0,5")) {
                Send_AT_Cmd("AT+QICSGP=1,1,\"cmnet\",\"\",\"\",1\r\n");
                g_modem_state = EC200M_STATE_CONFIG_APN;
                g_state_timer = 0;
            } else if (g_state_timer > 2000) {
                Send_AT_Cmd("AT+CEREG?\r\n");
                g_state_timer = 0;
            }
            break;

        case EC200M_STATE_CONFIG_APN:
            if (Check_Resp("OK")) {
                Send_AT_Cmd("AT+QIACT=1\r\n");
                g_modem_state = EC200M_STATE_ACTIVATE_PDP;
                g_state_timer = 0;
            } else if (g_state_timer > 3000) {
                g_modem_state = EC200M_STATE_CHECK_SIM; // 失败重头来
            }
            break;

        case EC200M_STATE_ACTIVATE_PDP:
            if (Check_Resp("OK") || Check_Resp("ERROR")) { // 可能已经激活过返回ERROR
                Send_AT_Cmd("AT+QIACT?\r\n");
                g_modem_state = EC200M_STATE_CHECK_IP;
                g_state_timer = 0;
            } else if (g_state_timer > 5000) {
                g_modem_state = EC200M_STATE_CHECK_SIM;
            }
            break;

        case EC200M_STATE_CHECK_IP:
            if (Check_Resp("+QIACT: 1,1,1,")) {
                if (g_work_mode == EC200M_MODE_TCP) {
                    char cmd[256];
                    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,1,\"TCP\",\"%s\",%s,0,1\r\n", TCP_SERVER_IP, TCP_SERVER_PORT);
                    Send_AT_Cmd(cmd);
                    g_modem_state = EC200M_STATE_TCP_OPEN;
                } else if (g_work_mode == EC200M_MODE_MQTT) {
                    char cmd[256];
                    snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=0,\"%s\",%s\r\n", MQTT_SERVER_IP, MQTT_SERVER_PORT);
                    Send_AT_Cmd(cmd);
                    g_modem_state = EC200M_STATE_MQTT_OPEN;
                }
                g_state_timer = 0;
            } else if (g_state_timer > 2000) {
                Send_AT_Cmd("AT+QIACT?\r\n");
                g_state_timer = 0;
            }
            break;

        /* ========== TCP 分支 ========== */
        case EC200M_STATE_TCP_OPEN:
            if (Check_Resp("+QIOPEN: 1,1,0")) { // 1=直推透传模式, 最后一个0代表成功
                Send_AT_Cmd("AT+QISEND=1\r\n");
                g_modem_state = EC200M_STATE_TCP_TRANSPARENT;
                g_state_timer = 0;
            } else if (g_state_timer > 10000) { // TCP连接可能比较慢
                g_modem_state = EC200M_STATE_CHECK_SIM;
            }
            break;
            
        case EC200M_STATE_TCP_TRANSPARENT:
            if (Check_Resp(">")) {
                printf("[TCP] Entered Transparent Mode!\r\n");
            }
            // 透传态，MCU可直接通过 EC200M_TCP_Send() 发送业务数据
            break;

        /* ========== MQTT 分支 ========== */
        case EC200M_STATE_MQTT_OPEN:
            if (Check_Resp("+QMTOPEN: 0,0")) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", MQTT_CLIENT_ID, MQTT_USER, MQTT_PWD);
                Send_AT_Cmd(cmd);
                g_modem_state = EC200M_STATE_MQTT_CONN;
                g_state_timer = 0;
            } else if (g_state_timer > 10000) {
                g_modem_state = EC200M_STATE_CHECK_SIM;
            }
            break;

        case EC200M_STATE_MQTT_CONN:
            if (Check_Resp("+QMTCONN: 0,0,0")) {
                Send_AT_Cmd("AT+QMTSUB=0,1,\"mppt/cmd\",1\r\n");
                g_modem_state = EC200M_STATE_MQTT_SUB;
                g_state_timer = 0;
            } else if (g_state_timer > 10000) {
                g_modem_state = EC200M_STATE_CHECK_SIM;
            }
            break;
            
        case EC200M_STATE_MQTT_SUB:
            if (Check_Resp("+QMTSUB: 0,1,0")) {
                printf("[MQTT] Connected and Subscribed!\r\n");
                g_modem_state = EC200M_STATE_MQTT_IDLE;
            } else if (g_state_timer > 5000) {
                g_modem_state = EC200M_STATE_CHECK_SIM;
            }
            break;
            
        case EC200M_STATE_MQTT_IDLE:
            /* 此状态下可以调用 EC200M_MQTT_Publish 发送数据 */
            /* 以及处理下发数据 +QMTRECV */
            if (g_ec200m_rx_flag) {
                if (strstr((const char*)g_ec200m_rx_buf, "+QMTRECV:")) {
                    printf("[MQTT RECV]: %s\n", g_ec200m_rx_buf);
                }
                g_ec200m_rx_flag = 0;
            }
            break;
            
        default:
            break;
    }
}
