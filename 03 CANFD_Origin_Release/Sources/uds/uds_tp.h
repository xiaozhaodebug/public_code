/**
  * @file    uds_tp.h
  * @brief   CAN TP层接口定义 (ISO 15765-2)
  * @details 定义CAN传输协议层接口，支持单帧、首帧、连续帧、流控帧处理
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_TP_H
#define UDS_TP_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * TP层常量定义
 *===========================================================================*/
#define UDS_TP_PCI_SF       0x00u   /* 单帧协议控制信息 */
#define UDS_TP_PCI_FF       0x10u   /* 首帧协议控制信息 */
#define UDS_TP_PCI_CF       0x20u   /* 连续帧协议控制信息 */
#define UDS_TP_PCI_FC       0x30u   /* 流控帧协议控制信息 */

#define UDS_TP_MAX_SF_LEN   7u      /* 经典CAN单帧最大数据长度 */
#define UDS_TP_MAX_FD_SF_LEN 62u    /* CAN FD单帧最大数据长度 */
#define UDS_TP_FF_DL_OFFSET 12u     /* 首帧12位长度偏移 */

/*=============================================================================
 * TP层接口函数
 *===========================================================================*/

/**
  * @brief      TP层初始化
  * @details    初始化TP层上下文，包括接收状态、发送状态、流控状态等
  * @return     void
  * @note       清除所有状态和缓冲区
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpInit(void);

/**
  * @brief      TP层接收处理
  * @details    根据PCI类型分发处理单帧、首帧、连续帧、流控帧
  * @param[in]  data   接收数据（包含PCI）
  * @param[in]  length 数据长度
  * @return     void
  * @note       必须在CAN接收中断中调用
  * @warning    在中断上下文中执行
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpRxIndication(uint8_t *data, uint8_t length);

/**
  * @brief      TP层发送请求
  * @details    发送数据，根据长度自动选择单帧或多帧发送
  * @param[in]  data   待发送数据
  * @param[in]  length 数据长度
  * @return     status_t 发送状态，STATUS_SUCCESS表示成功
  * @note       多帧发送需要等待流控帧
  * @warning    发送过程中不要修改数据缓冲区
  * @author     [小昭debug]
  * @date       2026-03-09
  */
status_t UdsTpTransmit(uint8_t *data, uint16_t length);

/**
  * @brief      TP层定时器处理
  * @details    处理接收超时、发送流控等待超时、连续帧间隔定时器
  * @return     void
  * @note       必须在1ms定时器中断中调用
  * @warning    在中断上下文中执行
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpTimerHandler(void);

/**
  * @brief      检查TP层是否正在接收
  * @details    检查是否处于多帧接收状态
  * @return     bool true表示正在接收，false表示空闲
  * @note       用于判断是否可以处理新消息
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsTpIsRxBusy(void);

/**
  * @brief      检查TP层是否正在发送
  * @details    检查是否处于多帧发送状态
  * @return     bool true表示正在发送，false表示空闲
  * @note       用于判断是否可以发起新发送
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsTpIsTxBusy(void);

/**
  * @brief      获取接收缓冲区数据
  * @details    获取接收完成的数据指针和长度
  * @param[out] length 数据长度指针，用于返回接收数据长度
  * @return     uint8_t* 数据指针，NULL表示无数据
  * @note       数据缓冲区在调用UdsTpClearRx前有效
  * @warning    不要修改返回的数据缓冲区内容
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t* UdsTpGetRxData(uint16_t *length);

/**
  * @brief      清除接收状态
  * @details    清除接收状态，准备接收下一条消息
  * @return     void
  * @note       处理完消息后必须调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpClearRx(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_TP_H */
