/**
  * @file    uds_security.h
  * @brief   UDS安全访问管理接口
  * @details 定义安全访问服务接口，包括种子生成、密钥验证、延时锁定等
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SECURITY_H
#define UDS_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 安全访问接口函数
 *===========================================================================*/

/**
  * @brief      安全访问初始化
  * @details    初始化安全状态为锁定，清除失败计数和延时定时器
  * @return     void
  * @note       在UdsInit中被调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityInit(void);

/**
  * @brief      配置安全访问等级
  * @details    设置安全访问等级配置表
  * @param[in]  config 安全等级配置表指针
  * @return     void
  * @note       配置表包含安全等级、最大尝试次数、延时时间、密钥算法函数
  * @warning    配置表指针必须在整个生命周期内有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityConfigInit(const UdsSecurityLevelConfig *config);

/**
  * @brief      获取安全状态
  * @details    返回当前安全访问状态（锁定/解锁/等待密钥）
  * @return     UdsSecurityState 当前安全状态
  * @note       状态包括LOCKED、UNLOCKED、WAIT_KEY
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsSecurityState UdsGetSecurityState(void);

/**
  * @brief      获取当前安全等级
  * @details    返回当前解锁的安全等级
  * @return     uint8_t 安全等级，0表示锁定
  * @note       解锁后返回解锁时的安全等级
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsGetSecurityLevel(void);

/**
  * @brief      解锁安全访问
  * @details    设置安全状态为解锁，保存安全等级，清除失败计数
  * @param[in]  level 安全等级
  * @return     void
  * @note       密钥验证成功后调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsUnlockSecurity(uint8_t level);

/**
  * @brief      锁定安全访问
  * @details    设置安全状态为锁定，清除安全等级
  * @return     void
  * @note       会话切换或S3超时时调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsLockSecurity(void);

/**
  * @brief      检查安全访问权限
  * @details    检查当前安全状态是否满足指定的安全等级要求
  * @param[in]  requiredLevel 所需安全等级
  * @return     bool          true表示已解锁且满足等级要求
  * @note       需要0级安全时直接返回true
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSecurityPermission(uint8_t requiredLevel);

/**
  * @brief      生成种子
  * @details    生成4字节随机数作为种子
  * @param[out] seed 种子缓冲区指针，至少4字节
  * @return     void
  * @note       种子用于客户端计算密钥
  * @warning    确保seed缓冲区至少有4字节空间
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityGenerateSeed(uint8_t *seed);

/**
  * @brief      验证密钥
  * @details    使用配置的算法验证客户端发送的密钥
  * @param[in]  key   密钥数据，4字节
  * @param[in]  level 安全等级
  * @return     bool  true表示密钥正确
  * @note       使用种子和算法计算预期密钥进行比较
  * @warning    失败次数过多会触发延时锁定
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsSecurityVerifyKey(uint8_t *key, uint8_t level);

/**
  * @brief      获取失败尝试次数
  * @details    返回密钥验证失败次数
  * @return     uint8_t 失败次数
  * @note       用于判断是否需要延时锁定
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsSecurityGetFailedAttempts(void);

/**
  * @brief      增加失败尝试计数
  * @details    失败次数加1，达到最大值时启动延时定时器
  * @return     void
  * @note       密钥验证失败时调用
  * @warning    超过最大尝试次数会触发延时锁定
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityIncrementFailedAttempts(void);

/**
  * @brief      安全访问定时器处理
  * @details    递减延时锁定定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityTimerHandler(void);

/**
  * @brief      检查是否处于延时锁定状态
  * @details    检查延时定时器是否大于0
  * @return     bool true表示处于延时锁定状态
  * @note       延时锁定期间不允许请求种子
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsSecurityIsDelayActive(void);

/**
  * @brief      获取剩余延时时间
  * @details    返回延时定时器当前值
  * @return     uint32_t 剩余延时时间(ms)
  * @note       用于客户端显示剩余等待时间
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint32_t UdsSecurityGetDelayTime(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SECURITY_H */
