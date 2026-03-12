/**
  * @file    uds_nrc.h
  * @brief   UDS否定响应处理接口
  * @details 定义否定响应发送接口和NRC描述获取接口
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_NRC_H
#define UDS_NRC_H

#include <stdint.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 否定响应接口函数
 *===========================================================================*/

/**
  * @brief      发送否定响应
  * @details    构建并发送3字节否定响应帧
  * @param[in]  serviceId 服务SID
  * @param[in]  nrc       否定响应码
  * @return     void
  * @note       否定响应格式：0x7F + serviceId + nrc
  * @warning    在中断上下文中谨慎调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendNegativeResponse(uint8_t serviceId, UdsNrcCode nrc);

/**
  * @brief      发送0x78 ResponsePending
  * @details    发送响应等待的否定响应，更新pending计数
  * @param[in]  serviceId 服务SID
  * @return     void
  * @note       表示服务正在处理中，客户端应继续等待
  * @warning    频繁发送0x78可能影响诊断性能
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendResponsePending(uint8_t serviceId);

/**
  * @brief      获取NRC描述字符串（调试用）
  * @details    根据NRC码返回对应的描述字符串
  * @param[in]  nrc 否定响应码
  * @return     const char* NRC描述字符串
  * @note       用于调试日志和诊断输出
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const char* UdsGetNrcDescription(UdsNrcCode nrc);

#ifdef __cplusplus
}
#endif

#endif /* UDS_NRC_H */
