/**
  * @file    uds_svc_31.h
  * @brief   例程控制服务 (0x31) 接口
  * @details 定义0x31例程控制服务的处理接口
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_31_H
#define UDS_SVC_31_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 0x31服务处理函数
 *===========================================================================*/

/**
  * @brief      例程控制主处理函数
  * @details    根据子功能（启动/停止/请求结果）和RID执行相应操作
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       通过RID配置表进行分发，支持动态配置
  * @warning    确保RID配置表已正确初始化
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc31Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_31_H */
