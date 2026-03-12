/**
  * @file    uds_svc_10.c
  * @brief   诊断会话控制服务 (0x10) 实现
  * @details 实现0x10诊断会话控制服务，支持默认、编程、扩展、安全会话切换
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_10.h"
#include "../uds_session.h"
#include "../uds_security.h"
#include "../uds_config.h"

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/*=============================================================================
 * 内部函数声明
 *===========================================================================*/
static UdsNrcCode UdsSvc10BuildResponse(uint8_t sessionType, 
                                         uint8_t *txData, uint16_t *txLen);

/**
  * @brief      构建0x10服务肯定响应
  * @details    构建包含会话类型、P2Server、P2*Server的响应数据
  * @param[in]  sessionType 会话类型
  * @param[out] txData      发送数据缓冲区
  * @param[out] txLen       发送数据长度指针
  * @return     UdsNrcCode  NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       响应格式：0x50 + subFunction + p2_high + p2_low + p2*_high + p2*_low
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static UdsNrcCode UdsSvc10BuildResponse(uint8_t sessionType, 
                                         uint8_t *txData, uint16_t *txLen)
{
    uint16_t p2Default;
    uint16_t p2Extended;
    
    /* 响应格式: 0x50, subFunction, p2_high, p2_low, p2*_high, p2*_low */
    txData[0] = 0x50;           /* 肯定响应SID */
    txData[1] = sessionType;    /* 子服务ID */
    
    /* P2 Server Default (50ms = 0x0032) */
    p2Default = UDS_P2_SERVER_DEFAULT;
    txData[2] = (uint8_t)(p2Default >> 8);
    txData[3] = (uint8_t)(p2Default & 0xFFu);
    
    /* P2* Server (5000ms = 0x1388) */
    p2Extended = UDS_P2_SERVER_EXTENDED;
    txData[4] = (uint8_t)(p2Extended >> 8);
    txData[5] = (uint8_t)(p2Extended & 0xFFu);
    
    *txLen = 6u;
    
    return UDS_NRC_GENERAL_REJECT;  /* 表示成功 */
}

/**
  * @brief      默认会话处理 (0x1001)
  * @details    切换到默认会话，锁定安全访问
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       切换到默认会话会锁定所有安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10DefaultSession(uint8_t *rxData, uint16_t rxLen, 
                                   uint8_t *txData, uint16_t *txLen)
{
    (void)rxData;
    
    /* 检查消息长度 */
    if (rxLen < 2u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 切换到默认会话 */
    UdsSetSession(UDS_SESSION_DEFAULT);
    
    /* 锁定安全访问 */
    UdsLockSecurity();
    
    /* 停止0x78等待 */
    /* UdsPendingStop(); */
    
    /* 构建响应 */
    return UdsSvc10BuildResponse(UDS_SVC_10_DEFAULT, txData, txLen);
}

/**
  * @brief      编程会话处理 (0x1002)
  * @details    切换到编程会话，锁定安全访问
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       编程会话用于软件刷写操作
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10ProgrammingSession(uint8_t *rxData, uint16_t rxLen, 
                                       uint8_t *txData, uint16_t *txLen)
{
    (void)rxData;
    
    /* 检查消息长度 */
    if (rxLen < 2u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 切换到编程会话 */
    UdsSetSession(UDS_SESSION_PROGRAMMING);
    
    /* 锁定安全访问 */
    UdsLockSecurity();
    
    /* 构建响应 */
    return UdsSvc10BuildResponse(UDS_SVC_10_PROGRAMMING, txData, txLen);
}

/**
  * @brief      扩展诊断会话处理 (0x1003)
  * @details    切换到扩展诊断会话
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       扩展会话允许执行更多诊断功能
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10ExtendedSession(uint8_t *rxData, uint16_t rxLen, 
                                    uint8_t *txData, uint16_t *txLen)
{
    (void)rxData;
    
    /* 检查消息长度 */
    if (rxLen < 2u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 切换到扩展诊断会话 */
    UdsSetSession(UDS_SESSION_EXTENDED);
    
    /* 构建响应 */
    return UdsSvc10BuildResponse(UDS_SVC_10_EXTENDED, txData, txLen);
}

/**
  * @brief      安全会话处理 (0x1004)
  * @details    切换到安全会话
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       安全会话提供最高级别的诊断权限
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10SecuritySession(uint8_t *rxData, uint16_t rxLen, 
                                    uint8_t *txData, uint16_t *txLen)
{
    (void)rxData;
    
    /* 检查消息长度 */
    if (rxLen < 2u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 切换到安全会话 */
    UdsSetSession(UDS_SESSION_SECURITY);
    
    /* 构建响应 */
    return UdsSvc10BuildResponse(UDS_SVC_10_SECURITY, txData, txLen);
}
