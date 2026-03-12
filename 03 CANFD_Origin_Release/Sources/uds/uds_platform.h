/**
 * @file uds_platform.h
 * @brief UDS平台抽象层接口声明
 * @note 用户需要实现这些底层接口函数
 */

#ifndef UDS_PLATFORM_H
#define UDS_PLATFORM_H

#include <stdint.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CAN报文发送函数
 * @note 用户需实现此函数，内部调用 CanFdSendQuick
 * 
 * @param[in] canId CAN ID
 * @param[in] data 数据指针
 * @param[in] length 数据长度
 * @return status_t 发送状态
 */
status_t UdsPlatformCanTransmit(uint32_t canId, uint8_t *data, uint8_t length);

/**
 * @brief 获取当前时间戳（毫秒）
 * @note 可选，用于调试或超时精确计算
 * 
 * @return uint32_t 当前毫秒计数
 */
uint32_t UdsPlatformGetTimeMs(void);

/**
 * @brief 随机数生成函数（用于安全访问种子）
 * @note 用户需实现安全的随机数生成
 * 
 * @return uint32_t 32位随机数
 */
uint32_t UdsPlatformRandomGenerate(void);

/**
 * @brief 系统复位函数
 * @note 0x11服务ECU复位时使用
 * 
 * @param[in] resetType 复位类型
 */
void UdsPlatformSystemReset(uint8_t resetType);

#ifdef __cplusplus
}
#endif

#endif /* UDS_PLATFORM_H */
