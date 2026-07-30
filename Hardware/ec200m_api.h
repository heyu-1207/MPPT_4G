#ifndef __EC200M_API_H
#define __EC200M_API_H

#include "stm32f10x.h"
#include <stdbool.h>

/* EC200M 模组状态枚举 */
typedef enum {
    EC200M_STATE_OFF = 0,
    EC200M_STATE_POWER_ON,
    EC200M_STATE_INIT_AT,
    EC200M_STATE_ECHO_OFF,
    EC200M_STATE_CHECK_SIM,
    EC200M_STATE_CHECK_CREG,
    EC200M_STATE_CHECK_CGREG,
    EC200M_STATE_CONFIG_APN,
    EC200M_STATE_ACTIVATE_PDP,
    EC200M_STATE_CHECK_IP,
    
    /* TCP 状态 */
    EC200M_STATE_TCP_OPEN,
    EC200M_STATE_TCP_TRANSPARENT,
    
    /* MQTT 状态 */
    EC200M_STATE_MQTT_OPEN,
    EC200M_STATE_MQTT_CONN,
    EC200M_STATE_MQTT_SUB,
    EC200M_STATE_MQTT_IDLE,
    
    EC200M_STATE_ERROR,
    EC200M_STATE_EXIT_TRANS_1,
    EC200M_STATE_EXIT_TRANS_2,
    EC200M_STATE_EXIT_TRANS_3
} EC200M_State_t;

/* 用户选择的工作模式 */
typedef enum {
    EC200M_MODE_NONE = 0,
    EC200M_MODE_TCP,
    EC200M_MODE_MQTT
} EC200M_Mode_t;

void EC200M_Init(void);
void EC200M_SetMode(EC200M_Mode_t mode);
void EC200M_Poll(uint32_t sys_tick);

/* MQTT 数据发布接口 */
bool EC200M_MQTT_Publish(const char *topic, const char *payload);

/* TCP 透传发送接口 */
bool EC200M_TCP_Send(const uint8_t *data, uint16_t len);

/* 获取当前状态 (用于外部判断是否已连接上) */
bool EC200M_IsConnected(void);

#endif /* __EC200M_API_H */
