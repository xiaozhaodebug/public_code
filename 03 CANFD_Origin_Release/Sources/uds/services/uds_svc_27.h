/**
  * @file    uds_svc_27.h
  * @brief   安全访问服务 (0x27) 接口
  * @details 定义0x27安全访问服务的子服务和处理函数
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_27_H
#define UDS_SVC_27_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 安全访问子服务ID
 *===========================================================================*/
#define UDS_SVC_27_REQUEST_SEED_L1  0x01    /* 请求种子 - 等级1 */
#define UDS_SVC_27_SEND_KEY_L1      0x02    /* 发送密钥 - 等级1 */

/*=============================================================================
 * 0x27服务处理函数
 *===========================================================================*/

/**
  * @brief      请求种子处理 (0x2701)
  * @details    生成4字节种子并返回给客户端
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       如果已解锁且等级足够，返回全0种子
  * @warning    延时锁定状态下返回NRC 0x37
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc27RequestSeed(uint8_t *rxData, uint16_t rxLen, 
                                uint8_t *txData, uint16_t *txLen);

/**
  * @brief      发送密钥处理 (0x2702)
  * @details    验证客户端发送的密钥，正确则解锁安全访问
  * @param[in]  rxData 接收数据缓冲区（包含密钥）
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       必须在请求种子后才能发送密钥
  * @warning    密钥错误会增加失败计数，达到最大值会触发延时锁定
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc27SendKey(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_27_H */
