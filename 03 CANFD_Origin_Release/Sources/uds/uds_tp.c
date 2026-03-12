/**
  * @file    uds_tp.c
  * @brief   CAN TP层实现 (ISO 15765-2)
  * @details 实现ISO 15765-2传输协议，支持单帧、多帧收发，流控管理
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_tp.h"
#include "uds_config.h"
#include "uds_platform.h"
#include <string.h>

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/*=============================================================================
 * 内部宏定义
 *===========================================================================*/
#define UDS_TP_GET_PCI_TYPE(pci)    ((pci) & 0xF0u)
#define UDS_TP_GET_SF_DL(pci)       ((pci) & 0x0Fu)
#define UDS_TP_GET_FF_DL_HIGH(pci)  ((pci) & 0x0Fu)
#define UDS_TP_GET_CF_SEQ(pci)      ((pci) & 0x0Fu)
#define UDS_TP_GET_FC_FS(pci)       ((pci) & 0x0Fu)
#define UDS_TP_GET_FC_BS(data)      ((data)[1])
#define UDS_TP_GET_FC_STMIN(data)   ((data)[2])

#define UDS_TP_FC_BS_DEFAULT        8u      /* 默认BlockSize */
#define UDS_TP_STMIN_DEFAULT        20u     /* 默认STmin (ms) */

/*=============================================================================
 * 内部函数声明
 *===========================================================================*/
static void UdsTpHandleSF(uint8_t *data, uint8_t length);
static void UdsTpHandleFF(uint8_t *data, uint8_t length);
static void UdsTpHandleCF(uint8_t *data, uint8_t length);
static void UdsTpHandleFC(uint8_t *data, uint8_t length);
static void UdsTpSendFC(uint8_t fs, uint8_t bs, uint8_t stmin);
static void UdsTpSendCF(void);
static uint8_t UdsTpGetSfMaxLen(void);
static uint16_t UdsTpCalcTxFrameLen(uint16_t remaining);

/**
  * @brief      TP层初始化
  * @details    清零接收状态、发送状态、流控状态等所有TP层变量
  * @return     void
  * @note       包括接收缓冲区、发送缓冲区、定时器等
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpInit(void)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    
    /* 清除接收状态 */
    ctx->rxInProgress = false;
    ctx->rxLength = 0u;
    ctx->rxIndex = 0u;
    ctx->rxExpectedSeq = 0u;
    ctx->rxBlockSize = 0u;
    ctx->rxTimer = 0u;
    
    /* 清除发送状态 */
    ctx->txInProgress = false;
    ctx->txLength = 0u;
    ctx->txIndex = 0u;
    ctx->txSeq = 0u;
    ctx->txBlockSize = 0u;
    ctx->txSTmin = UDS_TP_STMIN_DEFAULT;
    ctx->txTimer = 0u;
    ctx->txFcWaitTimer = 0u;
    
    /* 清除流控状态 */
    ctx->fcWait = false;
    ctx->fcStatus = UDS_FC_CTS;
}

/**
  * @brief      获取单帧最大数据长度
  * @details    根据CAN类型（经典CAN或CAN FD）返回单帧最大长度
  * @return     uint8_t 单帧最大数据长度
  * @note       经典CAN为7字节，CAN FD为62字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static uint8_t UdsTpGetSfMaxLen(void)
{
#if (UDS_TP_CAN_FD_ENABLE == 1)
    return UDS_TP_MAX_FD_SF_LEN;
#else
    return UDS_TP_MAX_SF_LEN;
#endif
}

/**
  * @brief      TP层接收处理
  * @details    根据PCI类型分发到单帧、首帧、连续帧、流控帧处理函数
  * @param[in]  data   接收数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       PCI类型由帧的第一个字节高4位决定
  * @warning    无效的PCI类型会被忽略
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpRxIndication(uint8_t *data, uint8_t length)
{
    uint8_t pci;
    
    if (data == NULL || length == 0u) {
        return;
    }
    
    pci = data[0];
    
    switch (UDS_TP_GET_PCI_TYPE(pci)) {
        case UDS_TP_PCI_SF:  /* 单帧 */
            UdsTpHandleSF(data, length);
            break;
            
        case UDS_TP_PCI_FF:  /* 首帧 */
            UdsTpHandleFF(data, length);
            break;
            
        case UDS_TP_PCI_CF:  /* 连续帧 */
            UdsTpHandleCF(data, length);
            break;
            
        case UDS_TP_PCI_FC:  /* 流控帧 */
            UdsTpHandleFC(data, length);
            break;
            
        default:
            /* 无效的PCI类型，忽略 */
            break;
    }
}

/**
  * @brief      处理单帧(SF)
  * @details    解析单帧数据长度，复制数据到接收缓冲区
  * @param[in]  data   单帧数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       单帧数据长度由PCI低4位决定
  * @warning    如果正在接收多帧，会中止当前接收
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpHandleSF(uint8_t *data, uint8_t length)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint8_t sfDl = UDS_TP_GET_SF_DL(data[0]);
    
    /* 检查数据长度有效性 */
    if (sfDl == 0u || sfDl > UdsTpGetSfMaxLen()) {
        return;
    }
    
    /* 如果正在接收多帧，中止当前接收 */
    if (ctx->rxInProgress) {
        ctx->rxInProgress = false;
    }
    
    /* 单帧数据直接存入缓冲区 */
    ctx->rxLength = sfDl;
    ctx->rxIndex = sfDl;
    (void)memcpy(ctx->rxBuffer, &data[1], sfDl);
    
    /* 数据接收完成，等待上层处理 */
    ctx->rxInProgress = false;
}

/**
  * @brief      处理首帧(FF)
  * @details    解析多帧数据总长度，初始化接收状态，发送流控帧CTS
  * @param[in]  data   首帧数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       首帧包含12位数据长度和6字节数据
  * @warning    如果正在接收其他多帧，会中止当前接收
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpHandleFF(uint8_t *data, uint8_t length)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint16_t ffDl;
    
    /* 计算首帧数据长度 */
    ffDl = ((uint16_t)(UDS_TP_GET_FF_DL_HIGH(data[0])) << 8) | data[1];
    
    /* 检查长度有效性 */
    if (ffDl < 8u || ffDl > UDS_TP_MAX_FRAME_SIZE) {
        return;
    }
    
    /* 如果正在接收，中止当前接收 */
    if (ctx->rxInProgress) {
        ctx->rxInProgress = false;
    }
    
    /* 初始化接收状态 */
    ctx->rxLength = ffDl;
    ctx->rxIndex = length - 2u;  /* 首帧已接收的数据 */
    ctx->rxExpectedSeq = 1u;     /* 期望下一个连续帧序列号为1 */
    ctx->rxInProgress = true;
    
    /* 复制首帧数据 (跳过2字节PCI) */
    (void)memcpy(ctx->rxBuffer, &data[2], ctx->rxIndex);
    
    /* 发送流控帧 (CTS, BlockSize=8, STmin=20ms) */
    UdsTpSendFC(UDS_FC_CTS, UDS_TP_FC_BS_DEFAULT, UDS_TP_STMIN_DEFAULT);
    
    /* 启动N_Cr定时器 */
    ctx->rxTimer = UDS_TP_N_CR;
}

/**
  * @brief      处理连续帧(CF)
  * @details    验证序列号，复制数据，检查接收完成，管理BlockSize
  * @param[in]  data   连续帧数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       序列号范围0-15，循环使用
  * @warning    序列号错误会中止接收
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpHandleCF(uint8_t *data, uint8_t length)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint8_t seq = UDS_TP_GET_CF_SEQ(data[0]);
    uint16_t copyLen;
    
    /* 检查是否正在接收 */
    if (!ctx->rxInProgress) {
        return;
    }
    
    /* 检查序列号 */
    if (seq != ctx->rxExpectedSeq) {
        /* 序列号错误，中止接收 */
        ctx->rxInProgress = false;
        return;
    }
    
    /* 计算本次复制长度 */
    copyLen = length - 1u;  /* 减去1字节PCI */
    if ((ctx->rxIndex + copyLen) > ctx->rxLength) {
        copyLen = ctx->rxLength - ctx->rxIndex;
    }
    
    /* 复制数据 */
    (void)memcpy(&ctx->rxBuffer[ctx->rxIndex], &data[1], copyLen);
    ctx->rxIndex += copyLen;
    
    /* 更新序列号 */
    ctx->rxExpectedSeq++;
    if (ctx->rxExpectedSeq > 15u) {
        ctx->rxExpectedSeq = 0u;
    }
    
    /* 检查是否接收完成 */
    if (ctx->rxIndex >= ctx->rxLength) {
        ctx->rxInProgress = false;
        /* 数据接收完成 */
        return;
    }
    
    /* 检查是否需要发送流控帧 */
    if ((ctx->rxExpectedSeq % UDS_TP_FC_BS_DEFAULT) == 0u) {
        UdsTpSendFC(UDS_FC_CTS, UDS_TP_FC_BS_DEFAULT, UDS_TP_STMIN_DEFAULT);
    }
    
    /* 重置N_Cr定时器 */
    ctx->rxTimer = UDS_TP_N_CR;
}

/**
  * @brief      处理流控帧(FC)
  * @details    解析流状态，更新发送参数，控制发送流程
  * @param[in]  data   流控帧数据
  * @param[in]  length 数据长度
  * @return     void
  * @note       支持CTS、WAIT、OVFLW三种流状态
  * @warning    如果不在等待流控状态会忽略该帧
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpHandleFC(uint8_t *data, uint8_t length)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint8_t fs;
    
    (void)length;
    
    /* 检查是否正在等待流控 */
    if (!ctx->txInProgress || !ctx->fcWait) {
        return;
    }
    
    fs = UDS_TP_GET_FC_FS(data[0]);
    
    switch (fs) {
        case UDS_FC_CTS:  /* 继续发送 */
            ctx->txBlockSize = UDS_TP_GET_FC_BS(data);
            ctx->txSTmin = UDS_TP_GET_FC_STMIN(data);
            ctx->fcWait = false;
            ctx->fcStatus = UDS_FC_CTS;
            
            /* 重置N_Bs定时器 */
            ctx->txFcWaitTimer = 0u;
            
            /* 发送第一个连续帧 */
            UdsTpSendCF();
            break;
            
        case UDS_FC_WAIT:  /* 等待 */
            ctx->fcStatus = UDS_FC_WAIT;
            /* 重置N_Bs定时器 */
            ctx->txFcWaitTimer = UDS_TP_N_BS;
            break;
            
        case UDS_FC_OVFLW:  /* 缓冲区溢出 */
        default:
            /* 中止发送 */
            ctx->txInProgress = false;
            ctx->fcWait = false;
            break;
    }
}

/**
  * @brief      发送流控帧(FC)
  * @details    构建并发送流控帧，指定流状态、BlockSize、STmin
  * @param[in]  fs     流状态(CTS/WAIT/OVFLW)
  * @param[in]  bs     BlockSize，连续帧数量
  * @param[in]  stmin  最小分离时间，单位ms
  * @return     void
  * @note       流控帧固定8字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpSendFC(uint8_t fs, uint8_t bs, uint8_t stmin)
{
    uint8_t fcFrame[8];
    
    fcFrame[0] = UDS_TP_PCI_FC | (fs & 0x0Fu);
    fcFrame[1] = bs;
    fcFrame[2] = stmin;
    fcFrame[3] = 0u;  /* 保留 */
    fcFrame[4] = 0u;
    fcFrame[5] = 0u;
    fcFrame[6] = 0u;
    fcFrame[7] = 0u;
    
    (void)UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, fcFrame, 8u);
}

/**
  * @brief      TP层发送请求
  * @details    根据数据长度选择单帧或多帧发送方式
  * @param[in]  data   待发送数据
  * @param[in]  length 数据长度
  * @return     status_t 发送状态
  * @note       单帧直接发送，多帧需要等待流控帧
  * @warning    发送过程中不要修改数据缓冲区
  * @author     [小昭debug]
  * @date       2026-03-09
  */
status_t UdsTpTransmit(uint8_t *data, uint16_t length)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint8_t sfMaxLen = UdsTpGetSfMaxLen();
    
    if (data == NULL || length == 0u || length > UDS_TP_MAX_FRAME_SIZE) {
        return STATUS_ERROR;
    }
    
    /* 检查是否正在发送 */
    if (ctx->txInProgress) {
        return STATUS_BUSY;
    }
    
    /* 复制数据到发送缓冲区 */
    (void)memcpy(ctx->txBuffer, data, length);
    ctx->txLength = length;
    ctx->txIndex = 0u;
    ctx->txSeq = 1u;  /* 连续帧从1开始 */
    
    if (length <= sfMaxLen) {
        /* 单帧发送 */
        uint8_t txFrame[64];
        txFrame[0] = UDS_TP_PCI_SF | (uint8_t)length;
        (void)memcpy(&txFrame[1], data, length);
        
        /* 经典CAN填充到8字节，CAN FD根据实际长度 */
#if (UDS_TP_CAN_FD_ENABLE == 0)
        for (uint8_t i = length + 1u; i < 8u; i++) {
            txFrame[i] = 0u;
        }
        return UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, txFrame, 8u);
#else
        return UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, txFrame, length + 1u);
#endif
    } else {
        /* 多帧发送 */
        uint8_t ffFrame[64];
        uint16_t ffDataLen;
        
        /* 构建首帧 */
        ffFrame[0] = UDS_TP_PCI_FF | (uint8_t)((length >> 8) & 0x0Fu);
        ffFrame[1] = (uint8_t)(length & 0xFFu);
        
        /* 计算首帧数据长度 */
        ffDataLen = UdsTpCalcTxFrameLen(length);
        (void)memcpy(&ffFrame[2], data, ffDataLen);
        
        ctx->txIndex = ffDataLen;
        ctx->txInProgress = true;
        ctx->fcWait = true;
        ctx->txFcWaitTimer = UDS_TP_N_BS;
        
        return UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, ffFrame, ffDataLen + 2u);
    }
}

/**
  * @brief      计算发送帧数据长度
  * @details    根据剩余数据量和CAN类型计算本次发送的数据长度
  * @param[in]  remaining 剩余数据量
  * @return     uint16_t 本次发送的数据长度
  * @note       经典CAN首帧最多6字节，CAN FD首帧最多62字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static uint16_t UdsTpCalcTxFrameLen(uint16_t remaining)
{
    (void)remaining;
    /* 首帧最多6字节数据 (经典CAN) */
    /* 首帧最多62字节数据 (CAN FD) */
#if (UDS_TP_CAN_FD_ENABLE == 1)
    if (remaining > 62u) {
        return 62u;
    }
    return remaining;
#else
    if (remaining > 6u) {
        return 6u;
    }
    return remaining;
#endif
}

/**
  * @brief      发送连续帧(CF)
  * @details    构建并发送一个连续帧，更新发送状态
  * @return     void
  * @note       根据STmin定时发送，序列号0-15循环
  * @warning    发送完成后检查是否还有数据需要发送
  * @author     [小昭debug]
  * @date       2026-03-09
  */
static void UdsTpSendCF(void)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    uint8_t cfFrame[64];
    uint16_t remaining = ctx->txLength - ctx->txIndex;
    uint16_t cfDataLen;
    uint8_t txLen;
    
    if (remaining == 0u) {
        ctx->txInProgress = false;
        return;
    }
    
    /* 构建连续帧PCI */
    cfFrame[0] = UDS_TP_PCI_CF | (ctx->txSeq & 0x0Fu);
    
    /* 计算本次发送数据长度 */
    cfDataLen = (remaining > 7u) ? 7u : remaining;
    (void)memcpy(&cfFrame[1], &ctx->txBuffer[ctx->txIndex], cfDataLen);
    
    ctx->txIndex += cfDataLen;
    ctx->txSeq++;
    if (ctx->txSeq > 15u) {
        ctx->txSeq = 0u;
    }
    
    /* 经典CAN填充到8字节 */
#if (UDS_TP_CAN_FD_ENABLE == 0)
    for (uint8_t i = cfDataLen + 1u; i < 8u; i++) {
        cfFrame[i] = 0u;
    }
    txLen = 8u;
#else
    txLen = cfDataLen + 1u;
#endif
    
    (void)UdsPlatformCanTransmit(UDS_PHYS_RESP_ADDR, cfFrame, txLen);
    
    /* 检查是否发送完成 */
    if (ctx->txIndex >= ctx->txLength) {
        ctx->txInProgress = false;
        ctx->fcWait = false;
        return;
    }
    
    /* 启动N_CS定时器 */
    ctx->txTimer = ctx->txSTmin;
}

/**
  * @brief      TP层定时器处理
  * @details    处理接收超时、发送流控等待超时、连续帧间隔定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @warning    在中断上下文中执行
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpTimerHandler(void)
{
    UdsTpContext *ctx = &gUdsContext.tpContext;
    
    /* 接收超时处理 */
    if (ctx->rxInProgress && ctx->rxTimer > 0u) {
        ctx->rxTimer--;
        if (ctx->rxTimer == 0u) {
            /* N_Cr超时，中止接收 */
            ctx->rxInProgress = false;
        }
    }
    
    /* 发送流控等待超时 */
    if (ctx->txInProgress && ctx->fcWait && ctx->txFcWaitTimer > 0u) {
        ctx->txFcWaitTimer--;
        if (ctx->txFcWaitTimer == 0u) {
            /* N_Bs超时，中止发送 */
            ctx->txInProgress = false;
            ctx->fcWait = false;
        }
    }
    
    /* 连续帧间隔定时器 */
    if (ctx->txInProgress && !ctx->fcWait && ctx->txTimer > 0u) {
        ctx->txTimer--;
        if (ctx->txTimer == 0u) {
            /* STmin到期，发送下一帧 */
            /* 检查BlockSize */
            if ((ctx->txSeq % ctx->txBlockSize) == 0u) {
                /* 需要等待新的流控帧 */
                ctx->fcWait = true;
                ctx->txFcWaitTimer = UDS_TP_N_BS;
            } else {
                UdsTpSendCF();
            }
        }
    }
}

/**
  * @brief      检查TP层是否正在接收
  * @details    检查rxInProgress标志
  * @return     bool true表示正在接收
  * @note       用于判断是否可以处理新消息
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsTpIsRxBusy(void)
{
    return gUdsContext.tpContext.rxInProgress;
}

/**
  * @brief      检查TP层是否正在发送
  * @details    检查txInProgress标志
  * @return     bool true表示正在发送
  * @note       用于判断是否可以发起新发送
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsTpIsTxBusy(void)
{
    return gUdsContext.tpContext.txInProgress;
}

/**
  * @brief      获取接收缓冲区数据
  * @details    返回接收缓冲区的指针和长度
  * @param[out] length 数据长度指针
  * @return     uint8_t* 数据指针
  * @note       数据在调用UdsTpClearRx前有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t* UdsTpGetRxData(uint16_t *length)
{
    if (length != NULL) {
        *length = gUdsContext.tpContext.rxLength;
    }
    return gUdsContext.tpContext.rxBuffer;
}

/**
  * @brief      清除接收状态
  * @details    清零接收状态和长度，准备接收下一条消息
  * @return     void
  * @note       处理完消息后必须调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsTpClearRx(void)
{
    gUdsContext.tpContext.rxInProgress = false;
    gUdsContext.tpContext.rxLength = 0u;
    gUdsContext.tpContext.rxIndex = 0u;
}
