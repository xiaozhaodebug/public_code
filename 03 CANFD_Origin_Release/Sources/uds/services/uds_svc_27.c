/**
  * @file    uds_svc_27.c
  * @brief   安全访问服务 (0x27) 实现
  * @details 实现0x27安全访问服务，包括种子生成和密钥验证
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_27.h"
#include "../uds_security.h"
#include "../uds_session.h"
#include "../uds_config.h"
#include <string.h>

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      请求种子处理 (0x2701)
  * @details    检查延时锁定状态，生成或返回种子
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       已解锁状态下返回全0种子
  * @warning    延时锁定状态下返回NRC 0x37
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc27RequestSeed(uint8_t *rxData, uint16_t rxLen, 
                                uint8_t *txData, uint16_t *txLen)
{
    uint8_t securityLevel;
    
    /* 检查消息长度 */
    if (rxLen < 2u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 获取安全等级 */
    securityLevel = rxData[1];
    
    /* 检查延时锁定状态 */
    if (UdsSecurityIsDelayActive()) {
        return UDS_NRC_REQUIRED_TIME_DELAY;
    }
    
    /* 检查是否已解锁 */
    if (UdsGetSecurityState() == UDS_SECURITY_UNLOCKED && 
        UdsGetSecurityLevel() >= securityLevel) {
        /* 已解锁，返回全0种子 */
        txData[0] = 0x67;           /* 肯定响应SID */
        txData[1] = UDS_SVC_27_REQUEST_SEED_L1;
        txData[2] = 0x00;
        txData[3] = 0x00;
        txData[4] = 0x00;
        txData[5] = 0x00;
        *txLen = 6u;
        return UDS_NRC_GENERAL_REJECT;  /* 成功 */
    }
    
    /* 生成新种子 */
    UdsSecurityGenerateSeed(gUdsContext.securitySeed);
    
    /* 设置等待密钥状态 */
    gUdsContext.securityState = UDS_SECURITY_WAIT_KEY;
    
    /* 构建响应 */
    txData[0] = 0x67;           /* 肯定响应SID */
    txData[1] = UDS_SVC_27_REQUEST_SEED_L1;
    txData[2] = gUdsContext.securitySeed[0];
    txData[3] = gUdsContext.securitySeed[1];
    txData[4] = gUdsContext.securitySeed[2];
    txData[5] = gUdsContext.securitySeed[3];
    *txLen = 6u;
    
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}

/**
  * @brief      发送密钥处理 (0x2702)
  * @details    验证客户端密钥，正确则解锁安全访问
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       密钥错误返回NRC 0x35并增加失败计数
  * @warning    必须在请求种子后发送密钥，否则返回NRC 0x24
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc27SendKey(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen)
{
    uint8_t securityLevel;
    uint8_t key[UDS_SECURITY_KEY_SIZE];
    
    /* 检查消息长度 */
    if (rxLen < 6u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 获取安全等级 */
    securityLevel = rxData[1];
    
    /* 检查延时锁定状态 */
    if (UdsSecurityIsDelayActive()) {
        return UDS_NRC_REQUIRED_TIME_DELAY;
    }
    
    /* 检查是否已解锁 */
    if (UdsGetSecurityState() == UDS_SECURITY_UNLOCKED && 
        UdsGetSecurityLevel() >= securityLevel) {
        /* 已解锁，返回肯定响应 */
        txData[0] = 0x67;           /* 肯定响应SID */
        txData[1] = UDS_SVC_27_SEND_KEY_L1;
        *txLen = 2u;
        return UDS_NRC_GENERAL_REJECT;  /* 成功 */
    }
    
    /* 检查是否处于等待密钥状态 */
    if (UdsGetSecurityState() != UDS_SECURITY_WAIT_KEY) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }
    
    /* 提取密钥 */
    key[0] = rxData[2];
    key[1] = rxData[3];
    key[2] = rxData[4];
    key[3] = rxData[5];
    
    /* 验证密钥 */
    if (UdsSecurityVerifyKey(key, securityLevel)) {
        /* 密钥正确，解锁 */
        UdsUnlockSecurity(securityLevel);
        
        /* 构建肯定响应 */
        txData[0] = 0x67;           /* 肯定响应SID */
        txData[1] = UDS_SVC_27_SEND_KEY_L1;
        *txLen = 2u;
        
        return UDS_NRC_GENERAL_REJECT;  /* 成功 */
    } else {
        /* 密钥错误 */
        UdsSecurityIncrementFailedAttempts();
        return UDS_NRC_INVALID_KEY;
    }
}
