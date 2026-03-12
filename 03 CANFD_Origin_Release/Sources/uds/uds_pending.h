/**
  * @file    uds_pending.h
  * @brief   UDS 0x78 ResponsePending 处理接口
  * @details 定义0x78响应等待处理接口，用于长耗时服务处理
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_PENDING_H
#define UDS_PENDING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 0x78 ResponsePending 接口函数
 *===========================================================================*/

/**
  * @brief      启动0x78响应等待
  * @details    激活pending状态，初始化计数器和定时器，保存服务ID
  * @param[in]  serviceId 正在处理的服务SID
  * @return     void
  * @note       服务处理时间较长时调用，通知客户端等待
  * @warning    必须在服务开始处理时调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingStart(uint8_t serviceId);

/**
  * @brief      停止0x78响应等待
  * @details    清除pending状态、计数、定时器和服务ID
  * @return     void
  * @note       服务处理完成或出错时调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingStop(void);

/**
  * @brief      检查是否正在发送0x78
  * @details    检查pending激活状态
  * @return     bool true表示正在发送0x78
  * @note       用于判断当前是否处于响应等待状态
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingIsActive(void);

/**
  * @brief      获取已发送0x78的次数
  * @details    返回当前pending计数
  * @return     uint8_t 发送次数
  * @note       用于判断是否超过最大次数限制
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsPendingGetCount(void);

/**
  * @brief      0x78定时器处理
  * @details    递减pending间隔定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingTimerHandler(void);

/**
  * @brief      检查是否需要发送0x78
  * @details    检查定时器是否到期
  * @return     bool true表示需要发送0x78
  * @note       在主循环中调用，定时器到期时返回true
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingNeedSend(void);

/**
  * @brief      检查是否超过最大次数
  * @details    比较当前计数和配置的最大次数
  * @return     bool true表示超过最大次数
  * @note       超过最大次数后应返回最终否定响应
  * @warning    超过最大次数可能导致服务失败
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingIsMaxCountReached(void);

/**
  * @brief      获取正在处理的服务ID
  * @details    返回保存的服务SID
  * @return     uint8_t 服务SID
  * @note       用于发送0x78时指定服务ID
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsPendingGetServiceId(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_PENDING_H */
