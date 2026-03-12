/**
  * @file    uds_svc_22.h
  * @brief   根据标识符读取数据服务 (0x22) 接口
  * @details 定义0x22服务的处理接口，支持通过DID读取数据
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_22_H
#define UDS_SVC_22_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 0x22服务处理函数
 *===========================================================================*/

/**
  * @brief      根据标识符读取数据主处理函数
  * @details    根据请求的DID读取对应数据，DID 0x0204返回固定数据 0x11 0x22 0x33 0x44 0x55
  * @param[in]  rxData 接收数据缓冲区 (包含SID + DID高 + DID低)
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       请求格式: 0x22 + DID_H + DID_L
  * @note       响应格式: 0x62 + DID_H + DID_L + Data
  * @warning    确保输入数据长度至少为3字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc22Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_22_H */
