/**
  * @file    uds_nrc.c
  * @brief   UDS否定响应处理实现
  * @details 实现否定响应发送和NRC描述获取功能
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_nrc.h"
#include "uds_config.h"
#include "uds_platform.h"

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      发送否定响应
  * @details    构建3字节否定响应帧并通过CAN发送
  * @param[in]  serviceId 请求的服务SID
  * @param[in]  nrc       否定响应码
  * @return     void
  * @note       响应格式：0x7F + SID + NRC
  * @warning    确保CAN发送缓冲区可用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendNegativeResponse(uint8_t serviceId, UdsNrcCode nrc)
{
    uint8_t response[3];
    
    /* 构建否定响应帧 */
    response[0] = 0x7Fu;        /* 否定响应服务SID */
    response[1] = serviceId;    /* 请求的服务SID */
    response[2] = (uint8_t)nrc; /* NRC */
    
    /* 通过TP层发送 */
    (void)UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, response, 3u);
}

/**
  * @brief      发送0x78 ResponsePending
  * @details    发送0x78 NRC并更新pending计数和定时器
  * @param[in]  serviceId 服务SID
  * @return     void
  * @note       增加pending计数，重置发送间隔定时器
  * @warning    超过最大次数后会停止发送并返回错误响应
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendResponsePending(uint8_t serviceId)
{
    /* 发送0x78 NRC */
    UdsSendNegativeResponse(serviceId, UDS_NRC_RESPONSE_PENDING);
    
    /* 更新pending计数和定时器 */
    if (gUdsContext.pendingResp.active) {
        gUdsContext.pendingResp.count++;
        gUdsContext.pendingResp.timer = UDS_NRC_78_INTERVAL;
    }
}

/**
  * @brief      获取NRC描述字符串
  * @details    根据NRC码返回对应的可读描述
  * @param[in]  nrc 否定响应码
  * @return     const char* NRC描述字符串
  * @note       用于调试日志输出，未知NRC返回"Unknown"
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const char* UdsGetNrcDescription(UdsNrcCode nrc)
{
    switch (nrc) {
        case UDS_NRC_GENERAL_REJECT:
            return "GeneralReject";
        case UDS_NRC_SERVICE_NOT_SUPPORTED:
            return "ServiceNotSupported";
        case UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED:
            return "SubFunctionNotSupported";
        case UDS_NRC_INCORRECT_MESSAGE_LENGTH:
            return "IncorrectMessageLength";
        case UDS_NRC_CONDITIONS_NOT_CORRECT:
            return "ConditionsNotCorrect";
        case UDS_NRC_REQUEST_SEQUENCE_ERROR:
            return "RequestSequenceError";
        case UDS_NRC_REQUEST_OUT_OF_RANGE:
            return "RequestOutOfRange";
        case UDS_NRC_SECURITY_ACCESS_DENIED:
            return "SecurityAccessDenied";
        case UDS_NRC_INVALID_KEY:
            return "InvalidKey";
        case UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS:
            return "ExceedNumberOfAttempts";
        case UDS_NRC_REQUIRED_TIME_DELAY:
            return "RequiredTimeDelay";
        case UDS_NRC_RESPONSE_PENDING:
            return "ResponsePending";
        case UDS_NRC_SUB_FUNC_NOT_SUPPORT_SESSION:
            return "SubFuncNotSupportInSession";
        case UDS_NRC_SERVICE_NOT_SUPPORT_SESSION:
            return "ServiceNotSupportInSession";
        default:
            return "Unknown";
    }
}
