/**
  * @file    uds_core.c
  * @brief   UDS核心层实现
  * @details 实现UDS协议栈核心功能，包括服务分发、权限检查、响应处理等
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_core.h"
#include "uds_tp.h"
#include "uds_session.h"
#include "uds_security.h"
#include "uds_pending.h"
#include "uds_nrc.h"
#include "uds_config.h"
#include <string.h>

/*=============================================================================
 * 全局上下文定义
 *===========================================================================*/
UdsContext gUdsContext;

/*=============================================================================
 * 内部函数声明
 *===========================================================================*/
static void UdsProcessMessage(uint8_t *data, uint16_t length);
static void UdsServiceDispatch(uint8_t *data, uint16_t length);
static bool UdsIsServiceSupported(uint8_t serviceId);

/**
  * @brief      UDS协议栈初始化
  * @details    清零全局上下文，保存服务配置，初始化各子模块
  * @param[in]  serviceTable 服务配置表
  * @param[in]  serviceCount 服务数量
  * @return     void
  * @note       初始化内容包括：上下文清零、服务表保存、TP层/会话层/安全访问层初始化
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsInit(const UdsServiceConfig *serviceTable, uint8_t serviceCount)
{
    /* 初始化全局上下文 */
    (void)memset(&gUdsContext, 0u, sizeof(UdsContext));
    
    /* 服务配置表 */
    gUdsContext.serviceTable = serviceTable;
    gUdsContext.serviceCount = serviceCount;
    
    /* 初始化各模块 */
    UdsTpInit();
    UdsSessionInit();
    UdsSecurityInit();
}

/**
  * @brief      CAN接收指示
  * @details    根据CAN ID判断寻址类型，传递给TP层，刷新S3定时器
  * @param[in]  canId  CAN ID
  * @param[in]  data   接收数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       物理寻址使用UDS_PHYS_REQ_ADDR，功能寻址使用UDS_FUNC_REQ_ADDR
  * @warning    在中断上下文中执行
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsCanRxIndication(uint32_t canId, uint8_t *data, uint8_t length)
{
    /* 判断寻址类型 */
    if (canId == UDS_PHYS_REQ_ADDR) {
        gUdsContext.addrType = UDS_ADDR_PHYSICAL;
    } else if (canId == UDS_FUNC_REQ_ADDR) {
        gUdsContext.addrType = UDS_ADDR_FUNCTIONAL;
    } else {
        return;  /* 不处理的CAN ID */
    }
    
    /* 传递给TP层处理 */
    UdsTpRxIndication(data, length);
    
    /* 刷新S3定时器 */
    UdsRefreshS3Timer();
}

/**
  * @brief      1ms定时器中断处理
  * @details    依次调用各模块的定时器处理函数
  * @return     void
  * @note       包括TP层、会话层、安全访问、0x78处理、P2定时器
  * @warning    在中断上下文中执行，需保持简短
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTimerISR(void)
{
    /* TP层定时器 */
    UdsTpTimerHandler();
    
    /* 会话定时器 */
    UdsSessionTimerHandler();
    
    /* 安全访问定时器 */
    UdsSecurityTimerHandler();
    
    /* 0x78定时器 */
    UdsPendingTimerHandler();
    
    /* P2定时器递减 */
    if (gUdsContext.p2Active && gUdsContext.p2Timer > 0u) {
        gUdsContext.p2Timer--;
    }
}

/**
  * @brief      UDS主处理循环
  * @details    检查TP层接收状态，处理消息，处理0x78响应发送
  * @return     void
  * @note       处理完成后会自动清除接收状态
  * @warning    不要在定时器中断中调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsMainTask(void)
{
    uint8_t *rxData;
    uint16_t rxLen;
    
    /* 检查TP层是否接收完成 */
    if (!UdsTpIsRxBusy() && (rxLen = 0u, rxData = UdsTpGetRxData(&rxLen)) != NULL) {
        if (rxLen > 0u) {
            UdsProcessMessage(rxData, rxLen);
            UdsTpClearRx();
        }
    }
    
    /* 处理0x78发送 */
    if (UdsPendingNeedSend()) {
        uint8_t serviceId = UdsPendingGetServiceId();
        
        /* 检查是否超过最大次数 */
        if (UdsPendingIsMaxCountReached()) {
            /* 发送NRC 0x78作为最终否定响应 */
            UdsSendNegativeResponse(serviceId, UDS_NRC_RESPONSE_PENDING);
            UdsPendingStop();
        } else {
            /* 发送0x78 */
            UdsSendResponsePending(serviceId);
            
            /* 重置P2*定时器 */
            gUdsContext.p2Timer = UDS_P2_SERVER_EXTENDED;
            gUdsContext.p2Extended = true;
        }
    }
}

/**
  * @brief      处理接收到的消息
  * @details    检查参数有效性，功能寻址过滤，启动P2定时器，调用服务分发
  * @param[in]  data   接收数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       功能寻址下不支持的服务会静默丢弃
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsProcessMessage(uint8_t *data, uint16_t length)
{
    uint8_t serviceId;
    
    if (data == NULL || length == 0u) {
        return;
    }
    
    serviceId = data[0];
    
    /* 功能寻址：不支持的服务静默丢弃 */
    if (gUdsContext.addrType == UDS_ADDR_FUNCTIONAL) {
        if (!UdsIsServiceSupported(serviceId)) {
            return;  /* 静默丢弃 */
        }
    }
    
    /* 启动P2定时器 */
    gUdsContext.p2Timer = UDS_P2_SERVER_DEFAULT;
    gUdsContext.p2Active = true;
    gUdsContext.p2Extended = false;
    
    /* 服务分发 */
    UdsServiceDispatch(data, length);
    
    /* 停止P2定时器 */
    gUdsContext.p2Active = false;
}

/**
  * @brief      检查服务是否支持
  * @details    遍历服务配置表查找指定的服务ID
  * @param[in]  serviceId 服务ID
  * @return     bool      true表示支持，false表示不支持
  * @note       在服务表为NULL时返回false
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static bool UdsIsServiceSupported(uint8_t serviceId)
{
    const UdsServiceConfig *config;
    uint8_t i;
    
    if (gUdsContext.serviceTable == NULL) {
        return false;
    }
    
    config = gUdsContext.serviceTable;
    
    for (i = 0u; i < gUdsContext.serviceCount; i++) {
        if (config->serviceId == serviceId) {
            return true;
        }
        config++;
    }
    
    return false;
}

/**
  * @brief      服务分发
  * @details    根据服务ID查找配置，检查权限，调用处理函数，处理响应
  * @param[in]  data   请求数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       处理肯定响应、否定响应、抑制响应等各种情况
  * @warning    处理函数需要正确设置txData和txLen
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsServiceDispatch(uint8_t *data, uint16_t length)
{
    const UdsServiceConfig *serviceConfig = NULL;
    const UdsSubServiceConfig *subServiceConfig = NULL;
    uint8_t serviceId;
    uint8_t subServiceId;
    uint8_t subServiceClean;
    UdsNrcCode nrc = UDS_NRC_SERVICE_NOT_SUPPORTED;
    uint8_t i;
    uint8_t txData[UDS_TP_MAX_FRAME_SIZE];
    uint16_t txLen = 0u;
    
    serviceId = data[0];
    
    /* 查找服务配置 */
    for (i = 0u; i < gUdsContext.serviceCount; i++) {
        if (gUdsContext.serviceTable[i].serviceId == serviceId) {
            serviceConfig = &gUdsContext.serviceTable[i];
            break;
        }
    }
    
    if (serviceConfig == NULL) {
        /* 服务不支持 */
        if (gUdsContext.addrType != UDS_ADDR_FUNCTIONAL) {
            UdsSendNegativeResponse(serviceId, UDS_NRC_SERVICE_NOT_SUPPORTED);
        }
        return;
    }
    
    /* 检查消息长度 */
    if (length < 2u) {
        UdsSendNegativeResponse(serviceId, UDS_NRC_INCORRECT_MESSAGE_LENGTH);
        return;
    }
    
    /* 提取子服务ID并检查SPR位 */
    subServiceId = data[1];
    subServiceClean = subServiceId & 0x7Fu;
    gUdsContext.suppressPosResp = ((subServiceId & 0x80u) != 0u);
    
    /* 查找子服务配置 */
    if (serviceConfig->subServiceTable != NULL) {
        for (i = 0u; i < serviceConfig->subServiceCount; i++) {
            if (serviceConfig->subServiceTable[i].subServiceId == subServiceClean) {
                subServiceConfig = &serviceConfig->subServiceTable[i];
                break;
            }
        }
    }
    
    if (subServiceConfig == NULL) {
        /* 尝试使用默认处理函数 */
        if (serviceConfig->defaultHandler != NULL) {
            /* 使用清理后的子服务ID继续处理 */
            data[1] = subServiceClean;
            nrc = serviceConfig->defaultHandler(data, length, txData, &txLen);
        } else {
            nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
        }
    } else {
        /* 检查会话权限 */
        if (!UdsCheckSessionPermission(subServiceConfig->sessionMask)) {
            nrc = UDS_NRC_SUB_FUNC_NOT_SUPPORT_SESSION;
        }
        /* 检查安全访问权限 */
        else if (subServiceConfig->needSecurity && 
                 !UdsCheckSecurityPermission(subServiceConfig->securityLevel)) {
            nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
        }
        /* 调用子服务处理函数 */
        else if (subServiceConfig->handler != NULL) {
            /* 使用清理后的子服务ID */
            data[1] = subServiceClean;
            nrc = subServiceConfig->handler(data, length, txData, &txLen);
        } else {
            nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
        }
    }
    
    /* 处理响应 */
    if (nrc == UDS_NRC_GENERAL_REJECT) {
        /* 服务处理正常完成 */
        if (!gUdsContext.suppressPosResp || gUdsContext.addrType == UDS_ADDR_PHYSICAL) {
            /* SPR=0 或 物理寻址：发送肯定响应 */
            if (txLen > 0u) {
                UdsSendPositiveResponse(serviceId, txData, txLen);
            } else {
                /* 默认响应：SID + 0x40 */
                uint8_t resp[2];
                resp[0] = serviceId + 0x40u;
                resp[1] = subServiceClean;
                UdsSendPositiveResponse(serviceId, resp, 2u);
            }
        }
        /* SPR=1 且 功能寻址：不发送响应 */
    } else if (nrc != UDS_NRC_RESPONSE_PENDING) {
        /* 发送否定响应 */
        if (gUdsContext.addrType != UDS_ADDR_FUNCTIONAL) {
            UdsSendNegativeResponse(serviceId, nrc);
        }
    }
}

/**
  * @brief      发送肯定响应
  * @details    通过TP层发送肯定响应数据帧
  * @param[in]  serviceId 服务SID
  * @param[in]  data      响应数据
  * @param[in]  length    数据长度
  * @return     void
  * @note       第一个字节已经是响应SID (SID + 0x40)
  * @warning    确保数据缓冲区有效且长度正确
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendPositiveResponse(uint8_t serviceId, uint8_t *data, uint16_t length)
{
    (void)serviceId;
    
    if (data == NULL || length == 0u) {
        return;
    }
    
    /* 第一个字节已经是响应SID (SID + 0x40) */
    (void)UdsTpTransmit(data, length);
}

/**
  * @brief      检查抑制肯定响应位
  * @details    检查子服务字节的最高位（bit7）是否为1
  * @param[in]  subService 子服务字节
  * @return     bool       true表示需要抑制肯定响应
  * @note       SPR位为bit7，为1时抑制肯定响应
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSuppressPosResp(uint8_t subService)
{
    return ((subService & 0x80u) != 0u);
}
