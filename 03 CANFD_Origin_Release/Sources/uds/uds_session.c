/**
  * @file    uds_session.c
  * @brief   UDS会话管理实现
  * @details 实现诊断会话状态机，包括会话切换、权限检查、S3超时处理
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_session.h"
#include "uds_config.h"

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      会话管理初始化
  * @details    设置当前会话为默认会话，关闭S3定时器
  * @return     void
  * @note       初始化后系统处于默认会话状态
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSessionInit(void)
{
    gUdsContext.currentSession = UDS_SESSION_DEFAULT;
    gUdsContext.s3Timer = 0u;
    gUdsContext.s3Active = false;
}

/**
  * @brief      获取当前会话
  * @details    从全局上下文中获取当前会话类型
  * @return     UdsSessionType 当前会话类型
  * @note       返回值范围为UDS_SESSION_DEFAULT到UDS_SESSION_SECURITY
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsSessionType UdsGetCurrentSession(void)
{
    return gUdsContext.currentSession;
}

/**
  * @brief      切换会话
  * @details    更新当前会话，根据会话类型管理S3定时器
  * @param[in]  session 目标会话类型
  * @return     void
  * @note       默认会话关闭S3定时器，非默认会话启动S3定时器
  * @warning    S3超时会导致自动返回默认会话
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSetSession(UdsSessionType session)
{
    /* 更新当前会话 */
    gUdsContext.currentSession = session;
    
    /* 根据会话类型管理S3定时器 */
    if (session == UDS_SESSION_DEFAULT) {
        /* 默认会话，关闭S3定时器 */
        gUdsContext.s3Timer = 0u;
        gUdsContext.s3Active = false;
    } else {
        /* 非默认会话，启动S3定时器 */
        gUdsContext.s3Timer = UDS_S3_SERVER_TIMEOUT;
        gUdsContext.s3Active = true;
    }
}

/**
  * @brief      检查会话权限
  * @details    检查当前会话是否在指定的会话掩码中
  * @param[in]  sessionMask 会话权限位图
  * @return     bool        true表示有权限
  * @note       通过位运算检查当前会话是否被允许
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSessionPermission(uint32_t sessionMask)
{
    uint32_t currentSessionBit;
    
    /* 计算当前会话的位 */
    currentSessionBit = (1u << (uint8_t)gUdsContext.currentSession);
    
    /* 检查是否匹配 */
    return ((sessionMask & currentSessionBit) != 0u);
}

/**
  * @brief      刷新S3定时器
  * @details    在非默认会话中刷新S3定时器，防止会话超时
  * @return     void
  * @note       在收到有效诊断请求时调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsRefreshS3Timer(void)
{
    /* 只在非默认会话时刷新 */
    if (gUdsContext.currentSession != UDS_SESSION_DEFAULT && gUdsContext.s3Active) {
        gUdsContext.s3Timer = UDS_S3_SERVER_TIMEOUT;
    }
}

/**
  * @brief      会话定时器处理
  * @details    递减S3定时器，超时后返回默认会话并锁定安全访问
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @warning    超时后会调用UdsLockSecurity锁定安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSessionTimerHandler(void)
{
    /* S3定时器递减 */
    if (gUdsContext.s3Active && gUdsContext.s3Timer > 0u) {
        gUdsContext.s3Timer--;
        
        if (gUdsContext.s3Timer == 0u) {
            /* S3超时，返回默认会话 */
            UdsSetSession(UDS_SESSION_DEFAULT);
            
            /* 同时锁定安全访问 */
            extern void UdsLockSecurity(void);
            UdsLockSecurity();
        }
    }
}

/**
  * @brief      检查S3是否超时
  * @details    检查S3定时器状态
  * @return     bool true表示S3已超时
  * @note       用于外部检查会话是否已超时返回默认会话
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsIsS3Timeout(void)
{
    return (gUdsContext.s3Active && gUdsContext.s3Timer == 0u);
}

/**
  * @brief      获取会话名称
  * @details    根据会话类型返回对应的字符串名称
  * @param[in]  session 会话类型
  * @return     const char* 会话名称字符串
  * @note       用于调试输出，未知会话返回"Unknown"
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const char* UdsGetSessionName(UdsSessionType session)
{
    switch (session) {
        case UDS_SESSION_DEFAULT:
            return "Default";
        case UDS_SESSION_PROGRAMMING:
            return "Programming";
        case UDS_SESSION_EXTENDED:
            return "Extended";
        case UDS_SESSION_SECURITY:
            return "Security";
        default:
            return "Unknown";
    }
}
