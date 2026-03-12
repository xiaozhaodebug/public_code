/**
  * @file    uds_session.h
  * @brief   UDS会话管理层接口
  * @details 定义UDS诊断会话管理接口，支持默认、编程、扩展、安全会话
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SESSION_H
#define UDS_SESSION_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 会话层接口函数
 *===========================================================================*/

/**
  * @brief      会话管理初始化
  * @details    初始化会话为默认会话，关闭S3定时器
  * @return     void
  * @note       在UdsInit中被调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSessionInit(void);

/**
  * @brief      获取当前会话
  * @details    返回当前的诊断会话类型
  * @return     UdsSessionType 当前会话类型
  * @note       返回值包括默认、编程、扩展、安全会话
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsSessionType UdsGetCurrentSession(void);

/**
  * @brief      切换会话
  * @details    切换到指定的诊断会话，管理S3定时器
  * @param[in]  session 目标会话类型
  * @return     void
  * @note       非默认会话会启动S3定时器，默认会话会关闭S3定时器
  * @warning    切换到默认会话会自动锁定安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSetSession(UdsSessionType session);

/**
  * @brief      检查会话权限
  * @details    检查当前会话是否在指定的会话掩码中
  * @param[in]  sessionMask 会话权限位图
  * @return     bool        true表示有权限，false表示无权限
  * @note       位图定义：bit0=默认，bit1=编程，bit2=扩展，bit3=安全
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSessionPermission(uint32_t sessionMask);

/**
  * @brief      刷新S3定时器
  * @details    在非默认会话中收到有效请求时刷新S3定时器
  * @return     void
  * @note       防止会话超时返回默认会话
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsRefreshS3Timer(void);

/**
  * @brief      会话定时器处理
  * @details    处理S3会话超时定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @warning    S3超时会自动返回默认会话并锁定安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSessionTimerHandler(void);

/**
  * @brief      检查S3是否超时
  * @details    检查S3定时器是否已超时
  * @return     bool true表示已超时
  * @note       用于外部检查会话状态
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsIsS3Timeout(void);

/**
  * @brief      获取会话名称（调试用）
  * @details    返回会话类型的可读字符串
  * @param[in]  session 会话类型
  * @return     const char* 会话名称字符串
  * @note       用于调试打印和日志输出
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const char* UdsGetSessionName(UdsSessionType session);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SESSION_H */
