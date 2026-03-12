/**
  * @file    uds_svc_31.c
  * @brief   例程控制服务 (0x31) 实现
  * @details 实现0x31例程控制服务，采用配置表驱动方式支持RID动态配置
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_31.h"
#include "../uds_routine.h"
#include "../uds_core.h"
#include "../uds_nrc.h"
#include <string.h>

/*=============================================================================
 * 内部常量定义
 *===========================================================================*/
#define UDS_SVC31_MIN_LENGTH        4u   /* 最小长度: SID + subFunc + RID(2B) */

/**
  * @brief      0x31服务主处理函数
  * @details    解析子功能和RID，检查权限，调用相应的例程回调
  * @param[in]  rxData 接收数据
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据
  * @param[out] txLen  发送数据长度
  * @return     UdsNrcCode NRC码
  * @note       支持启动(0x01)、停止(0x02)、请求结果(0x03)三种操作
  * @warning    确保txData缓冲区足够大
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc31Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen)
{
    uint8_t routineType;
    uint16_t rid;
    const UdsRoutineConfig *routineConfig;
    UdsNrcCode nrc = UDS_NRC_GENERAL_REJECT;
    uint16_t respLen = 0u;
    
    if (rxData == NULL || txData == NULL || txLen == NULL) {
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
    
    /* 检查最小长度 */
    if (rxLen < UDS_SVC31_MIN_LENGTH) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 提取子功能（例程控制类型） */
    routineType = rxData[1];
    
    /* 提取RID (2字节, Big Endian) */
    rid = (uint16_t)(((uint16_t)rxData[2] << 8u) | (uint16_t)rxData[3]);
    
    /* 查找RID配置 */
    routineConfig = UdsRoutineFind(rid);
    if (routineConfig == NULL) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 检查RID权限 */
    if (!UdsRoutineCheckPermission(routineConfig)) {
        if (!UdsCheckSessionPermission(routineConfig->sessionMask)) {
            return UDS_NRC_SUB_FUNC_NOT_SUPPORT_SESSION;
        }
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    
    /* 根据例程控制类型调用相应回调 */
    switch (routineType) {
        case UDS_ROUTINE_START:
            if (routineConfig->routineStart == NULL) {
                nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
            } else {
                nrc = routineConfig->routineStart(&rxData[4], (uint16_t)(rxLen - 4u), 
                                                   txData, &respLen);
            }
            break;
            
        case UDS_ROUTINE_STOP:
            if (routineConfig->routineStop == NULL) {
                nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
            } else {
                nrc = routineConfig->routineStop(&rxData[4], (uint16_t)(rxLen - 4u), 
                                                  txData, &respLen);
            }
            break;
            
        case UDS_ROUTINE_REQUEST:
            if (routineConfig->routineRequest == NULL) {
                nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
            } else {
                nrc = routineConfig->routineRequest(&rxData[4], (uint16_t)(rxLen - 4u), 
                                                     txData, &respLen);
            }
            break;
            
        default:
            nrc = UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
            break;
    }
    
    /* 设置响应长度 */
    if (nrc == UDS_NRC_GENERAL_REJECT && respLen == 0u) {
        /* 默认响应: 0x71 + routineType + RID */
        txData[0] = 0x71;
        txData[1] = routineType;
        txData[2] = (uint8_t)(rid >> 8u);
        txData[3] = (uint8_t)(rid & 0xFFu);
        respLen = 4u;
    }
    
    *txLen = respLen;
    return nrc;
}
