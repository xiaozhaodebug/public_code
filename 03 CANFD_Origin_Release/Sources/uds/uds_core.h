/**
  * @file    uds_core.h
  * @brief   UDS核心层接口定义
  * @details 定义UDS协议栈核心接口，包括初始化、消息处理、定时器管理等
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_CORE_H
#define UDS_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 全局上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/*=============================================================================
 * 核心接口函数
 *===========================================================================*/

/**
  * @brief      UDS协议栈初始化
  * @details    初始化UDS全局上下文、TP层、会话层、安全访问层等模块
  * @param[in]  serviceTable 服务配置表指针，包含所有支持的服务配置
  * @param[in]  serviceCount 服务数量
  * @return     void
  * @note       必须在系统启动时调用，且只调用一次
  * @warning    调用前需确保serviceTable已正确配置且在生命周期内有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsInit(const UdsServiceConfig *serviceTable, uint8_t serviceCount);

/**
  * @brief      CAN接收指示函数
  * @details    在CAN接收中断中调用，判断寻址类型并传递给TP层处理
  * @param[in]  canId  CAN ID，用于判断物理寻址或功能寻址
  * @param[in]  data   接收数据缓冲区指针
  * @param[in]  length 数据长度
  * @return     void
  * @note       必须在CAN接收中断中调用
  * @warning    此函数在中断上下文中执行，需保持简短
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsCanRxIndication(uint32_t canId, uint8_t *data, uint8_t length);

/**
  * @brief      1ms定时器中断处理
  * @details    处理TP层定时器、会话定时器、安全访问定时器、0x78定时器等
  * @return     void
  * @note       必须在1ms定时器中断中调用
  * @warning    此函数在中断上下文中执行，需保持简短
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTimerISR(void);

/**
  * @brief      发送肯定响应
  * @details    通过TP层发送肯定响应数据
  * @param[in]  serviceId 服务SID（已包含+0x40的响应SID）
  * @param[in]  data      响应数据缓冲区指针
  * @param[in]  length    数据长度
  * @return     void
  * @note       第一个字节应为响应SID（请求SID + 0x40）
  * @warning    确保数据缓冲区在发送完成前有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsSendPositiveResponse(uint8_t serviceId, uint8_t *data, uint16_t length);

/**
  * @brief      检查抑制肯定响应位(SPR)
  * @details    检查子服务字节的最高位是否为1（抑制肯定响应）
  * @param[in]  subService 子服务字节
  * @return     bool       true表示需要抑制肯定响应，false表示正常响应
  * @note       SPR位为子服务字节的bit7
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsCheckSuppressPosResp(uint8_t subService);

/**
  * @brief      UDS主处理循环
  * @details    在主循环中调用，处理UDS状态机，包括消息处理、0x78响应发送等
  * @return     void
  * @note       需要周期性调用，建议周期不超过10ms
  * @warning    不要在定时器中断中调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsMainTask(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_CORE_H */
