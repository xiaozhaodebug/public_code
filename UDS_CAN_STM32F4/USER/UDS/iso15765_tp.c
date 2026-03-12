#include "iso15765_tp.h"
#include "can1.h"    /* 引入底层 CAN 发送函数 Can1SendMsgWithId */
#include "uds_app.h" /* UDS应用层处理函数包含 */

TpHandler tpRxHandler;

/**
  * @brief      初始化TP网络层
  * @details    复位网络层接收状态机中的所有字段，并注册上层回调函数(UdsProcessMsg)。
  *             当TP层重组出完整的UDS数据包后，会自动调用该回调进行处理。
  * @return     无
  * @note       需要在系统复位后调用一次，且应在 UdsInit() 之前调用。
  * @warning    未调用此函数将导致无法接收任何UDS长报文。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpInit(void) {
    tpRxHandler.state = TP_STATE_IDLE;
    tpRxHandler.rxTotalLen = 0;
    tpRxHandler.rxIndex = 0;
    tpRxHandler.expectedSn = 0;
    tpRxHandler.RxCompleteCallback = UdsProcessMsg;
}

/**
  * @brief      发送流控帧 (Flow Control)
  * @details    在收到首帧(FF)后，向测试仪发送流控帧(FC)以控制连续帧(CF)的发送节奏。
  *             FC帧格式: Byte0 = 0x30|FS, Byte1 = BS, Byte2 = STmin。
  * @param[in]  fs 流控状态 (0x00: CTS继续发送, 0x01: WAIT, 0x02: Overflow)
  * @param[in]  bs 块大小限制 (0表示无限制，一口气全发完)
  * @param[in]  stmin 连续帧发送最小间隔 (单位: ms)
  * @return     无
  * @note       发送时使用物理响应ID(0x75C)。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpSendFlowControl(uint8_t fs, uint8_t bs, uint8_t stmin) {
    uint8_t txBuf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    txBuf[0] = N_PCI_FC | (fs & 0x0F);
    txBuf[1] = bs;
    txBuf[2] = stmin;
    
    Can1SendMsgWithId(UDS_RESP_ID_PHYS, txBuf, 8); 
}

/**
  * @brief      发送单帧 (Single Frame)
  * @details    当响应的数据负载<=7字节时，使用单帧直接通过物理响应ID发出。
  *             SF帧格式: Byte0 = 0x00|len, Byte1~7 = 有效数据。
  * @param[in]  data 待发送的有效负载数据指针
  * @param[in]  len 发送的数据长度 (最大7字节)
  * @return     无
  * @note       发送时使用物理响应ID(0x75C)。
  * @warning    不要传入大于7的len，超出部分会被截断。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpSendSingleFrame(uint8_t *data, uint8_t len) {
    uint8_t txBuf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
    uint8_t i;
    
    txBuf[0] = N_PCI_SF | (len & 0x0F); 
    
    for(i = 0; i < len; i++) {
        txBuf[i+1] = data[i];
    }
    
    Can1SendMsgWithId(UDS_RESP_ID_PHYS, txBuf, 8);
}

/**
  * @brief      TP层数据接收入口 (状态机驱动)
  * @details    解析从CAN接收到的底层8字节数据（包含N_PCI信息），
  *             根据帧类型(SF/FF/CF/FC)驱动状态机进行长数据重组。
  *             当完整的UDS数据包重组完成后，自动调用上层回调函数。
  * @param[in]  data 指向接收到的8字节CAN数据
  * @param[in]  canDlc 实际收到数据的长度(通常是8)
  * @return     无
  * @note       应被放置于CAN接收中断(CAN1_RX0_IRQHandler)中调用。
  * @author     小昭debug
  * @date       2026-02-26
  */
void TpReceiveData(uint8_t *data, uint8_t canDlc) {
    uint8_t frameType; 
    uint8_t i;
    uint8_t dataLen; 
    uint16_t totalLen;
    uint8_t sn;
    uint16_t remainLen;
    uint8_t copyLen;
    
    if(canDlc == 0) return;
    
    frameType = data[0] & 0xF0; 
    
    switch (frameType) {
        /* ========== 1. 单帧 (Single Frame) ========== */
        case N_PCI_SF: 
        {
            dataLen = data[0] & 0x0F;
            
            if (dataLen > 7) { 
                break;
            }
            
            tpRxHandler.state = TP_STATE_IDLE;
            tpRxHandler.rxTotalLen = dataLen;
            tpRxHandler.rxIndex = 0;
            for(i = 0; i < dataLen; i++) {
                tpRxHandler.rxBuf[i] = data[i+1];
            }
            
            if(tpRxHandler.RxCompleteCallback != 0) {
                tpRxHandler.RxCompleteCallback(tpRxHandler.rxBuf, tpRxHandler.rxTotalLen);
            }
            break;
        }

        /* ========== 2. 首帧 (First Frame) ========== */
        case N_PCI_FF:
        {
            totalLen = ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
            
            if(totalLen > TP_MAX_BUF_SIZE) {
                tpRxHandler.state = TP_STATE_IDLE; 
                break;
            }
            
            tpRxHandler.rxTotalLen = totalLen;
            tpRxHandler.rxIndex = 0;
            tpRxHandler.expectedSn = 1;
            
            for(i = 0; i < 6; i++) {
                tpRxHandler.rxBuf[tpRxHandler.rxIndex++] = data[i+2];
            }
            
            tpRxHandler.state = TP_STATE_RX_PROGRESS;
            
            /* 收到首帧后立即回复流控帧, BS=0(不限), STmin=10ms */
            TpSendFlowControl(FC_FS_CTS, 0, 10); 
            break;
        }

        /* ========== 3. 连续帧 (Consecutive Frame) ========== */
        case N_PCI_CF:
        {
            if (tpRxHandler.state != TP_STATE_RX_PROGRESS) {
                break;
            }
            
            sn = data[0] & 0x0F;
            
            if (sn != tpRxHandler.expectedSn) {
                 tpRxHandler.state = TP_STATE_IDLE;
                 break;
            }
            
            remainLen = tpRxHandler.rxTotalLen - tpRxHandler.rxIndex;
            copyLen = (remainLen > 7) ? 7 : (uint8_t)remainLen;
            
            for (i = 0; i < copyLen; i++) {
                tpRxHandler.rxBuf[tpRxHandler.rxIndex++] = data[i+1];
            }
            
            tpRxHandler.expectedSn = (tpRxHandler.expectedSn + 1) & 0x0F;
            
            if (tpRxHandler.rxIndex >= tpRxHandler.rxTotalLen) {
                tpRxHandler.state = TP_STATE_IDLE;
                
                if(tpRxHandler.RxCompleteCallback != 0) {
                    tpRxHandler.RxCompleteCallback(tpRxHandler.rxBuf, tpRxHandler.rxTotalLen);
                }
            }
            break;
        }
        
        /* ========== 4. 流控帧 (Flow Control) ========== */
        case N_PCI_FC:
        {
            /* ECU作为发送方收到测试仪的FC时处理, 暂未实现 */
            break;
        }
        
        default:
            break;
    }
}
