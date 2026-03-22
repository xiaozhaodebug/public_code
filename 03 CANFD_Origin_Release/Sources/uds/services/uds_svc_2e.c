/**
  * @file    uds_svc_2e.c
  * @brief   根据标识符写入数据服务 (0x2E) 实现
  * @details 实现0x2E服务，支持通过DID写入数据
  *          - DID 0xF1F0: DTC故障注入控制，用于测试DTC上报功能
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_2e.h"
#include "../uds_dtc.h"
#include "../uds_session.h"
#include <string.h>

/*=============================================================================
 * 内部常量定义
 *===========================================================================*/
#define UDS_SVC2E_MIN_LENGTH        3u   /* 最小长度: SID + DID(2B) */

/*=============================================================================
 * 内部函数
 *===========================================================================*/

/**
 * @brief 处理DTC故障注入请求
 * @param data 数据缓冲区 [Magic] [DTC_H] [DTC_M] [DTC_L]
 * @param len 数据长度
 * @param trigger true=触发故障, false=清除故障
 * @return NRC码
 */
static UdsNrcCode DtcFaultHandler(uint8_t *data, uint16_t len, bool trigger)
{
    uint32_t dtcCode;
    
    /* 检查数据长度: Magic + DTC(3B) */
    if (len < 4u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 验证魔术字节 */
    if (data[0] != UDS_DTC_CONTROL_MAGIC) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 解析DTC代码 (3字节) */
    dtcCode = ((uint32_t)data[1] << 16u) |
              ((uint32_t)data[2] << 8u) |
              (uint32_t)data[3];
    
    /* 报告DTC状态 */
    if (UdsDtcReport(dtcCode, trigger)) {
        return UDS_NRC_GENERAL_REJECT;  /* 成功 */
    }
    return UDS_NRC_CONDITIONS_NOT_CORRECT;
}

/*=============================================================================
 * 0x2E服务主处理函数
 *===========================================================================*/

/**
 * @brief 0x2E服务主处理函数
 * @details 根据DID分发到对应的处理函数
 */
UdsNrcCode UdsSvc2eHandler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen)
{
    uint16_t did;
    UdsNrcCode result;
    UdsSessionType currentSession;
    
    if (rxData == NULL || txData == NULL || txLen == NULL) {
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
    
    /* 检查最小长度 (至少要有 SID + DID) */
    if (rxLen < 3u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 检查会话 - 0x2E服务需要扩展会话或更高 */
    currentSession = UdsGetCurrentSession();
    if (currentSession != UDS_SESSION_EXTENDED && 
        currentSession != UDS_SESSION_SECURITY) {
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
    
    /* 提取DID (2字节, Big Endian) */
    /* rxData[0] = SID (由TP层去除PCI后传递) */
    /* rxData[1] = DID_H */
    /* rxData[2] = DID_L */
    did = (uint16_t)(((uint16_t)rxData[1] << 8u) | (uint16_t)rxData[2]);
    
    /* 调试: 打印接收到的数据 */
    /* printf("0x2E: rxLen=%d, SID=0x%02X, DID=0x%04X\n", rxLen, rxData[0], did); */
    
    /* 根据DID处理 */
    switch (did) {
        case UDS_DID_DTC_TRIGGER:
            /* 触发DTC故障: [SID=2E] [F1] [F0] [Magic] [DTC_H] [DTC_M] [DTC_L] */
            /* rxData[3] = Magic, rxData[4..6] = DTC */
            result = DtcFaultHandler(&rxData[3], rxLen - 3u, true);
            if (result == UDS_NRC_GENERAL_REJECT) {
                /* 构建肯定响应: [SID+0x40=6E] [DID_H] [DID_L] */
                txData[0] = 0x6E;  /* 肯定响应SID */
                txData[1] = (uint8_t)(did >> 8u);   /* DID高字节 */
                txData[2] = (uint8_t)(did & 0xFFu); /* DID低字节 */
                *txLen = 3u;
            }
            return result;
            
        case UDS_DID_DTC_CLEAR:
            /* 清除指定DTC: [SID=2E] [F1] [F1] [Magic] [DTC_H] [DTC_M] [DTC_L] */
            result = DtcFaultHandler(&rxData[3], rxLen - 3u, false);
            if (result == UDS_NRC_GENERAL_REJECT) {
                txData[0] = 0x6E;
                txData[1] = (uint8_t)(did >> 8u);
                txData[2] = (uint8_t)(did & 0xFFu);
                *txLen = 3u;
            }
            return result;
            
        case UDS_DID_DTC_CLEAR_ALL:
            /* 清除所有DTC: [SID=2E] [F1] [F2] [Magic] */
            if (rxLen < 4u || rxData[3] != UDS_DTC_CONTROL_MAGIC) {
                return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
            }
            if (UdsDtcClearAll()) {
                txData[0] = 0x6E;
                txData[1] = (uint8_t)(did >> 8u);
                txData[2] = (uint8_t)(did & 0xFFu);
                *txLen = 3u;
                return UDS_NRC_GENERAL_REJECT;  /* 成功 */
            }
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
            
        default:
            /* 不支持的DID */
            return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
}
