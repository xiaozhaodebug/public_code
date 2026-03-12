/**
  * @file    uds_security.c
  * @brief   UDS安全访问管理实现
  * @details 实现安全访问服务，包括种子生成、密钥验证、延时锁定机制
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_security.h"
#include "uds_config.h"
#include "uds_platform.h"
#include <string.h>

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      安全访问初始化
  * @details    设置安全状态为锁定，清除安全等级、失败计数、延时定时器和种子
  * @return     void
  * @note       初始化后系统处于完全锁定状态
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityInit(void)
{
    gUdsContext.securityState = UDS_SECURITY_LOCKED;
    gUdsContext.securityLevel = 0u;
    gUdsContext.failedAttempts = 0u;
    gUdsContext.securityDelayTimer = 0u;
    (void)memset(gUdsContext.securitySeed, 0u, UDS_SECURITY_SEED_SIZE);
}

/**
  * @brief      配置安全访问等级
  * @details    保存安全等级配置表指针
  * @param[in]  config 安全等级配置表
  * @return     void
  * @note       配置表包含安全等级参数和密钥算法函数
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityConfigInit(const UdsSecurityLevelConfig *config)
{
    gUdsContext.securityConfig = config;
}

/**
  * @brief      获取安全状态
  * @details    从全局上下文中获取当前安全状态
  * @return     UdsSecurityState 当前安全状态
  * @note       返回值包括LOCKED、UNLOCKED、WAIT_KEY
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsSecurityState UdsGetSecurityState(void)
{
    return gUdsContext.securityState;
}

/**
  * @brief      获取当前安全等级
  * @details    返回当前解锁的安全等级
  * @return     uint8_t 安全等级，0表示锁定
  * @note       仅在UNLOCKED状态下有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsGetSecurityLevel(void)
{
    return gUdsContext.securityLevel;
}

/**
  * @brief      解锁安全访问
  * @details    设置安全状态为UNLOCKED，保存安全等级，清除失败计数
  * @param[in]  level 安全等级
  * @return     void
  * @note       密钥验证成功后调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsUnlockSecurity(uint8_t level)
{
    gUdsContext.securityState = UDS_SECURITY_UNLOCKED;
    gUdsContext.securityLevel = level;
    gUdsContext.failedAttempts = 0u;  /* 清除失败计数 */
}

/**
  * @brief      锁定安全访问
  * @details    设置安全状态为LOCKED，清除安全等级
  * @return     void
  * @note       会话切换、S3超时或复位时调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsLockSecurity(void)
{
    gUdsContext.securityState = UDS_SECURITY_LOCKED;
    gUdsContext.securityLevel = 0u;
}

/**
  * @brief      检查安全访问权限
  * @details    检查当前安全状态是否满足指定的安全等级要求
  * @param[in]  requiredLevel 所需安全等级
  * @return     bool          true表示有权限
  * @note       需要0级时直接返回true，其他情况需要已解锁且等级足够
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSecurityPermission(uint8_t requiredLevel)
{
    /* 需要0级安全（不需要安全访问） */
    if (requiredLevel == 0u) {
        return true;
    }
    
    /* 检查是否已解锁 */
    if (gUdsContext.securityState != UDS_SECURITY_UNLOCKED) {
        return false;
    }
    
    /* 检查安全等级是否满足要求 */
    return (gUdsContext.securityLevel >= requiredLevel);
}

/**
  * @brief      生成种子
  * @details    使用平台随机数生成器生成4字节种子
  * @param[out] seed 种子缓冲区指针
  * @return     void
  * @note       种子用于客户端计算密钥，本身不加密
  * @warning    确保seed缓冲区至少有4字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityGenerateSeed(uint8_t *seed)
{
    uint32_t randomValue;
    
    if (seed == NULL) {
        return;
    }
    
    /* 生成随机数 */
    randomValue = UdsPlatformRandomGenerate();
    
    /* 转换为字节数组 */
    seed[0] = (uint8_t)(randomValue >> 24);
    seed[1] = (uint8_t)(randomValue >> 16);
    seed[2] = (uint8_t)(randomValue >> 8);
    seed[3] = (uint8_t)(randomValue);
}

/**
  * @brief      验证密钥
  * @details    使用配置的算法计算预期密钥并与客户端密钥比较
  * @param[in]  key   客户端密钥，4字节
  * @param[in]  level 安全等级
  * @return     bool  true表示密钥正确
  * @note       使用当前保存的种子和配置的安全算法计算预期密钥
  * @warning    验证失败会增加失败计数，可能触发延时锁定
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsSecurityVerifyKey(uint8_t *key, uint8_t level)
{
    uint32_t seedValue;
    uint32_t keyValue;
    uint32_t expectedKey;
    
    if (key == NULL || gUdsContext.securityConfig == NULL) {
        return false;
    }
    
    /* 检查安全等级配置 */
    if (gUdsContext.securityConfig->level != level) {
        return false;
    }
    
    /* 检查是否有算法函数 */
    if (gUdsContext.securityConfig->calcKeyFunc == NULL) {
        return false;
    }
    
    /* 提取种子值 */
    seedValue = ((uint32_t)gUdsContext.securitySeed[0] << 24) |
                ((uint32_t)gUdsContext.securitySeed[1] << 16) |
                ((uint32_t)gUdsContext.securitySeed[2] << 8) |
                ((uint32_t)gUdsContext.securitySeed[3]);
    
    /* 提取密钥值 */
    keyValue = ((uint32_t)key[0] << 24) |
               ((uint32_t)key[1] << 16) |
               ((uint32_t)key[2] << 8) |
               ((uint32_t)key[3]);
    
    /* 使用配置的算法计算预期密钥 */
    expectedKey = gUdsContext.securityConfig->calcKeyFunc(seedValue);
    
    return (keyValue == expectedKey);
}

/**
  * @brief      获取失败尝试次数
  * @details    返回当前失败计数
  * @return     uint8_t 失败次数
  * @note       用于外部查询失败次数
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsSecurityGetFailedAttempts(void)
{
    return gUdsContext.failedAttempts;
}

/**
  * @brief      增加失败尝试计数
  * @details    失败次数加1，达到最大值时启动延时定时器
  * @return     void
  * @note       超过最大尝试次数会触发延时锁定
  * @warning    延时锁定期间不允许请求种子
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityIncrementFailedAttempts(void)
{
    gUdsContext.failedAttempts++;
    
    /* 检查是否超过最大尝试次数 */
    if (gUdsContext.securityConfig != NULL) {
        if (gUdsContext.failedAttempts >= gUdsContext.securityConfig->maxAttempts) {
            /* 启动延时定时器 */
            gUdsContext.securityDelayTimer = gUdsContext.securityConfig->delayTime;
        }
    }
}

/**
  * @brief      安全访问定时器处理
  * @details    递减延时锁定定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSecurityTimerHandler(void)
{
    /* 延时定时器递减 */
    if (gUdsContext.securityDelayTimer > 0u) {
        gUdsContext.securityDelayTimer--;
    }
}

/**
  * @brief      检查是否处于延时锁定状态
  * @details    检查延时定时器是否大于0
  * @return     bool true表示处于延时锁定
  * @note       延时锁定期间请求种子返回NRC 0x37
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsSecurityIsDelayActive(void)
{
    return (gUdsContext.securityDelayTimer > 0u);
}

/**
  * @brief      获取剩余延时时间
  * @details    返回延时定时器当前值
  * @return     uint32_t 剩余延时时间(ms)
  * @note       用于显示或记录剩余等待时间
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint32_t UdsSecurityGetDelayTime(void)
{
    return gUdsContext.securityDelayTimer;
}
