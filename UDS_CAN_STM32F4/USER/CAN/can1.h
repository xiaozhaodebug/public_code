#ifndef __CAN1_H
#define __CAN1_H
#include "common.h"

/* CAN1接收RX0中断使能 0:不使能;1:使能 */
#define CAN1_RX0_INT_ENABLE     1                                                

/**
  * @brief      初始化CAN1外设模式
  * @details    配置CAN1的GPIO引脚，波特率，并配置过滤器
  * @param[in]  mode CAN工作模式 (0:普通模式, 1:环回模式)
  * @return     u8 0代表初始化成功，其他代表失败
  * @note       当前配置为500Kbps
  * @author     Clever
  * @date       2017-08-30 (Modified 2026-02-26)
  */
u8 CAN1_Mode_Init(u8 mode);                                                                       

/**
  * @brief      通过CAN1发送数据帧（默认ID 0x12）
  * @details    向外发送固定标准ID0x12的数据帧
  * @param[in]  msg 数据指针，最大8字节
  * @param[in]  len 数据长度
  * @return     u8 0:成功, 1:失败
  * @author     Clever
  * @date       2017-08-30
  */
u8 CAN1_Send_Msg(u8* msg,u8 len);                                               

/**
  * @brief      通过CAN1发送指定ID的数据帧
  * @details    允许指定任意的标准帧ID进行数据发送
  * @param[in]  id 标准帧ID (例如 UDS物理响应 0x75C)
  * @param[in]  msg 数据指针，最大8字节
  * @param[in]  len 数据长度
  * @return     u8 0:成功, 1:失败
  * @note       UDS响应需使用此函数
  * @author     小昭debug
  * @date       2026-02-26
  */
u8 Can1SendMsgWithId(u32 id, u8* msg, u8 len);

/**
  * @brief      查询方式接收CAN1数据
  * @details    查询FIFO0是否有数据，有则取出
  * @param[out] buf 数据缓存区指针
  * @return     u8 0:无数据, 其他:接收的数据长度
  * @author     Clever
  * @date       2017-08-30
  */
u8 CAN1_Receive_Msg(u8 *buf);                                                   

#endif
