#ifndef __ISO15765_TP_H
#define __ISO15765_TP_H

#include "stm32f4xx.h"  

/* =======================
 * CAN TP 状态机定义
 * ======================= */
typedef enum {
    TP_STATE_IDLE = 0,
    TP_STATE_WAIT_FOR_FC,    /* 发送(长数据)方：等待流控帧 */
    TP_STATE_WAIT_FOR_CF,    /* 接收(长数据)方：等待连续帧 */
    TP_STATE_TX_PROGRESS,    /* 发送进行中(正在发CF) */
    TP_STATE_RX_PROGRESS     /* 接收进行中(正在收CF) */
} TpState;

/* =======================
 * N_PCI (网络层协议控制信息) 类型
 * ======================= */
#define N_PCI_SF    0x00  /* 单帧 (Single Frame) */
#define N_PCI_FF    0x10  /* 首帧 (First Frame) */
#define N_PCI_CF    0x20  /* 连续帧 (Consecutive Frame) */
#define N_PCI_FC    0x30  /* 流控帧 (Flow Control) */

/* 流控帧状态 FS (Flow Status) */
#define FC_FS_CTS   0x00  /* Continue To Send */
#define FC_FS_WAIT  0x01
#define FC_FS_OVFLW 0x02

/* =======================
 * 诊断 CAN ID 定义 (物理/功能寻址)
 * ======================= */
#define UDS_REQ_ID_PHYS  0x74C
#define UDS_RESP_ID_PHYS 0x75C
#define UDS_REQ_ID_FUNC  0x7DF

/* =======================
 * 配置项
 * ======================= */
#define TP_MAX_BUF_SIZE  1024  /* 最大支持的接收/发送数据长度 */

/* =======================
 * 全局处理结构体
 * ======================= */
typedef struct {
    TpState state;
    uint8_t rxBuf[TP_MAX_BUF_SIZE];
    uint16_t rxTotalLen;
    uint16_t rxIndex;
    uint8_t expectedSn;   /* 期望接收的 Consecutive Frame 序列号 (Sequence Number) */
    
    /* UDS 上层处理的回调函数 */
    void (*RxCompleteCallback)(uint8_t *data, uint16_t len);
} TpHandler;

extern TpHandler tpRxHandler;

/**
  * @brief      初始化TP网络层
  * @details    复位网络层接收状态机，注册回调函数。
  * @return     无
  * @note       需要在系统复位后调用一次。
  * @warning    未调用此函数将导致无法接收长报文。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpInit(void);

/**
  * @brief      TP层数据接收入口
  * @details    解析从CAN接收到的底层8字节数据（包含N_PCI信息），并驱动状态机重组长数据。
  * @param[in]  data 指向接收到的8字节CAN数据
  * @param[in]  canDlc 实际收到数据的长度(通常是8)
  * @return     无
  * @note       应被放置于CAN接收中断或轮询任务中。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpReceiveData(uint8_t *data, uint8_t canDlc);

/**
  * @brief      发送流控帧
  * @details    在收到首帧(FF)后向测试仪发送流控帧(FC)，以控制连续帧(CF)的发送节奏。
  * @param[in]  fs 流控状态 (通常为0: CTS 继续发送)
  * @param[in]  bs 块大小限制 (0表示无限制)
  * @param[in]  stmin 连续帧发送最小间隔
  * @return     无
  * @note       发送时使用物理响应ID(0x75C)
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpSendFlowControl(uint8_t fs, uint8_t bs, uint8_t stmin);

/**
  * @brief      发送单帧(Single Frame)
  * @details    当响应的数据负载<=7字节时，使用单帧直接通过物理响应ID发出。
  * @param[in]  data 待发送的有效负载数据指针
  * @param[in]  len 发送的数据长度(最大7)
  * @return     无
  * @note       发送时使用物理响应ID(0x75C)
  * @warning    不要传入大于7的len，否则会被截断。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpSendSingleFrame(uint8_t *data, uint8_t len);

#endif
