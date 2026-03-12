/**
  * @file    uds_svc_22.c
  * @brief   根据标识符读取数据服务 (0x22) 实现
  * @details 实现0x22服务，支持DID 0x0204读取固定数据 0x11 0x22 0x33 0x44 0x55
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_22.h"
#include <string.h>

/*=============================================================================
 * 内部常量定义
 *===========================================================================*/
#define UDS_SVC22_MIN_LENGTH        3u   /* 最小长度: SID + DID(2B) */

/* DID 0x0204 固定数据定义 */
#define UDS_DID_0204                0x0204u
#define UDS_DID_0204_DATA_LEN       5u

/* 固定数据内容: 0x11 0x22 0x33 0x44 0x55 */
static const uint8_t kDid0204Data[UDS_DID_0204_DATA_LEN] = {
    0x11, 0x22, 0x33, 0x44, 0x55
};

/*=============================================================================
 * 0x22服务主处理函数
 *===========================================================================*/
/**
  * @brief      根据标识符读取数据主处理函数
  * @details    解析DID，根据DID返回对应数据。当前支持DID 0x0204返回固定数据
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       DID 0x0204: 返回 5 字节固定数据 0x11 0x22 0x33 0x44 0x55
  * @warning    不支持的DID返回 NRC_REQUEST_OUT_OF_RANGE
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc22Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen)
{
    uint16_t did;
    
    if (rxData == NULL || txData == NULL || txLen == NULL) {
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
    
    /* 检查最小长度 */
    if (rxLen < UDS_SVC22_MIN_LENGTH) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 提取DID (2字节, Big Endian) */
    did = (uint16_t)(((uint16_t)rxData[1] << 8u) | (uint16_t)rxData[2]);
    
    /* 根据DID处理 */
    switch (did) {
        case UDS_DID_0204:
            /* 构建响应: 0x62 + DID_H + DID_L + Data */
            txData[0] = 0x62;                       /* 肯定响应SID */
            txData[1] = (uint8_t)(did >> 8u);       /* DID高字节 */
            txData[2] = (uint8_t)(did & 0xFFu);     /* DID低字节 */
            /* 复制固定数据 */
            (void)memcpy(&txData[3], kDid0204Data, UDS_DID_0204_DATA_LEN);
            *txLen = 3u + UDS_DID_0204_DATA_LEN;    /* 3字节头 + 5字节数据 */
            return UDS_NRC_GENERAL_REJECT;          /* 成功 */
            
        default:
            /* 不支持的DID */
            return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
}
