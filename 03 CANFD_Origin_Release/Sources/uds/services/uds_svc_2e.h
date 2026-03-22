/**
  * @file    uds_svc_2e.h
  * @brief   根据标识符写入数据服务 (0x2E) 接口
  * @details 实现0x2E服务，支持通过DID写入数据，用于触发/清除DTC故障测试
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_2E_H
#define UDS_SVC_2E_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 特殊DID定义（用于DTC故障注入测试）
 * 请求格式（单帧，最多8字节）：
 *   [PCI Len] [0x2E] [DID_H] [DID_L] [Magic] [DTC_H] [DTC_M] [DTC_L]
 *   PCI Len = 6 (SID + DID + Magic + DTC[3])
 *===========================================================================*/
#define UDS_DID_DTC_TRIGGER         0xF1F0u  /* 触发DTC故障 */
#define UDS_DID_DTC_CLEAR           0xF1F1u  /* 清除指定DTC */
#define UDS_DID_DTC_CLEAR_ALL       0xF1F2u  /* 清除所有DTC */
#define UDS_DTC_CONTROL_MAGIC       0xA5u    /* 激活故障注入的魔术字节 */

/*=============================================================================
 * 0x2E服务处理函数
 *===========================================================================*/

/**
 * @brief 0x2E服务主处理函数
 * @param[in] rxData 接收数据缓冲区
 * @param[in] rxLen  接收数据长度
 * @param[out] txData 发送数据缓冲区
 * @param[out] txLen  发送数据长度指针
 * @return UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
 * @note 请求格式: 0x2E + DID_H + DID_L + Data[]
 * @note 响应格式: 0x6E + DID_H + DID_L
 * @note 支持DID:
 *       - 0xF1F0: DTC故障注入控制
 *         Data[0] = 0xA5 (魔术字节)
 *         Data[1] = DTC代码高字节
 *         Data[2] = DTC代码中字节
 *         Data[3] = DTC代码低字节
 *         Data[4] = 0x01(触发故障) / 0x00(清除故障)
 */
UdsNrcCode UdsSvc2eHandler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_2E_H */
