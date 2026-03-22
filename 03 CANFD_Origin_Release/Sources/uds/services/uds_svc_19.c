/**
  * @file    uds_svc_19.c
  * @brief   读取DTC信息服务 (0x19) 实现
  * @details 实现0x19服务，支持以下子服务：
  *          - 0x02: 按状态掩码报告DTC列表
  *          - 0x0A: 报告支持的DTC
  *          - 0x06: 报告DTC扩展数据
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_svc_19.h"
#include "../uds_dtc.h"
#include <string.h>

/*=============================================================================
 * 内部常量定义
 *===========================================================================*/
#define UDS_SVC19_MIN_LENGTH            2u   /* 最小长度: SID + SubFunction */
#define UDS_SVC19_RESP_DTC_LIST         0x59u  /* 肯定响应SID */
#define UDS_DTC_FORMAT                  UDS_DTC_FORMAT_ISO14229  /* DTC格式 */

/*=============================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 将3字节DTC编码到缓冲区 (Big Endian)
 * @param dtcCode DTC代码
 * @param[out] buffer 输出缓冲区
 */
static void DtcEncode(uint32_t dtcCode, uint8_t *buffer)
{
    buffer[0] = (uint8_t)((dtcCode >> 16u) & 0xFFu);
    buffer[1] = (uint8_t)((dtcCode >> 8u) & 0xFFu);
    buffer[2] = (uint8_t)(dtcCode & 0xFFu);
}

/**
 * @brief 从缓冲区解码3字节DTC (Big Endian)
 * @param buffer 输入缓冲区
 * @return DTC代码
 */
static uint32_t DtcDecode(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0] << 16u) |
           ((uint32_t)buffer[1] << 8u) |
           (uint32_t)buffer[2];
}

/*=============================================================================
 * 0x19服务主处理函数
 *===========================================================================*/

/**
 * @brief 0x19服务主处理函数
 * @details 根据子服务ID分发到对应的处理函数
 */
UdsNrcCode UdsSvc19Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen)
{
    uint8_t subService;
    
    if (rxData == NULL || txData == NULL || txLen == NULL) {
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
    
    /* 检查最小长度 */
    if (rxLen < UDS_SVC19_MIN_LENGTH) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 提取子服务ID (清除SPR位) */
    subService = rxData[1] & 0x7Fu;
    
    /* 根据子服务分发 */
    switch (subService) {
        case UDS_SVC19_REPORT_DTC_BY_STATUS:
            /* 0x02: 按状态掩码报告DTC列表 */
            return UdsSvc19ReportDtcByStatus(rxData, rxLen, txData, txLen);
            
        case UDS_SVC19_REPORT_SUPPORTED_DTC:
            /* 0x0A: 报告支持的DTC */
            return UdsSvc19ReportSupportedDtc(rxData, rxLen, txData, txLen);
            
        case UDS_SVC19_REPORT_DTC_EXT_DATA:
            /* 0x06: 报告DTC扩展数据 */
            return UdsSvc19ReportDtcExtData(rxData, rxLen, txData, txLen);
            
        default:
            /* 不支持的子服务 */
            return UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
    }
}

/*=============================================================================
 * 子服务实现：0x02 - 按状态掩码报告DTC列表
 *===========================================================================*/

/**
 * @brief 0x02子服务：按状态掩码报告DTC列表
 * @details 请求格式: 0x19 0x02 [StatusMask]
 *          响应格式: 0x59 0x02 [DTCFormat] [DTCCount] [DTC1_H] [DTC1_M] [DTC1_L] [Status1] ...
 */
UdsNrcCode UdsSvc19ReportDtcByStatus(uint8_t *rxData, uint16_t rxLen, 
                                      uint8_t *txData, uint16_t *txLen)
{
    uint8_t statusMask;
    uint32_t dtcList[UDS_DTC_MAX_COUNT];
    uint8_t dtcCount = UDS_DTC_MAX_COUNT;
    uint8_t i;
    uint16_t respIndex;
    UdsDtcEntry *entry;
    
    /* 检查请求长度: SID + SubFunction + StatusMask */
    if (rxLen < 3u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 获取状态掩码 */
    statusMask = rxData[2];
    
    /* 获取匹配的DTC列表 */
    (void)UdsDtcGetListByStatus(statusMask, dtcList, &dtcCount);
    
    /* 构建响应 */
    respIndex = 0u;
    txData[respIndex++] = UDS_SVC19_RESP_DTC_LIST;  /* 0x59 */
    txData[respIndex++] = UDS_SVC19_REPORT_DTC_BY_STATUS;  /* 0x02 */
    txData[respIndex++] = UDS_DTC_FORMAT;            /* DTC格式 */
    
    /* 返回DTC数量 (1字节) */
    txData[respIndex++] = dtcCount;
    
    /* 填充DTC列表和状态 */
    for (i = 0u; i < dtcCount && (respIndex + 4u) <= UDS_TP_MAX_FRAME_SIZE; i++) {
        /* DTC代码 (3字节) */
        DtcEncode(dtcList[i], &txData[respIndex]);
        respIndex += 3u;
        
        /* 状态字节 */
        entry = UdsDtcFindEntry(dtcList[i]);
        if (entry != NULL) {
            txData[respIndex++] = entry->status;
        } else {
            txData[respIndex++] = 0u;
        }
    }
    
    *txLen = respIndex;
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}

/*=============================================================================
 * 子服务实现：0x0A - 报告支持的DTC
 *===========================================================================*/

/**
 * @brief 0x0A子服务：报告支持的DTC
 * @details 请求格式: 0x19 0x0A
 *          响应格式: 0x59 0x0A [DTCFormat] [AvailabilityMask] [DTC1_H] [DTC1_M] [DTC1_L] [Status1] ...
 */
UdsNrcCode UdsSvc19ReportSupportedDtc(uint8_t *rxData, uint16_t rxLen, 
                                       uint8_t *txData, uint16_t *txLen)
{
    uint32_t dtcList[UDS_DTC_MAX_COUNT];
    uint8_t dtcCount = UDS_DTC_MAX_COUNT;
    uint8_t i;
    uint16_t respIndex;
    UdsDtcEntry *entry;
    
    (void)rxData;  /* 未使用 */
    (void)rxLen;   /* 未使用，0x0A不需要额外参数 */
    
    /* 获取所有支持的DTC列表 */
    (void)UdsDtcGetSupportedList(dtcList, &dtcCount);
    
    /* 构建响应 */
    respIndex = 0u;
    txData[respIndex++] = UDS_SVC19_RESP_DTC_LIST;  /* 0x59 */
    txData[respIndex++] = UDS_SVC19_REPORT_SUPPORTED_DTC;  /* 0x0A */
    txData[respIndex++] = UDS_DTC_FORMAT;            /* DTC格式 */
    
    /* 可用性掩码 (表示支持的状态位) */
    txData[respIndex++] = 0xFFu;  /* 支持所有状态位 */
    
    /* 填充DTC列表和状态 */
    for (i = 0u; i < dtcCount && (respIndex + 4u) <= UDS_TP_MAX_FRAME_SIZE; i++) {
        /* DTC代码 (3字节) */
        DtcEncode(dtcList[i], &txData[respIndex]);
        respIndex += 3u;
        
        /* 状态字节 */
        entry = UdsDtcFindEntry(dtcList[i]);
        if (entry != NULL) {
            txData[respIndex++] = entry->status;
        } else {
            txData[respIndex++] = 0u;
        }
    }
    
    *txLen = respIndex;
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}

/*=============================================================================
 * 子服务实现：0x06 - 报告DTC扩展数据
 *===========================================================================*/

/**
 * @brief 0x06子服务：报告DTC扩展数据
 * @details 请求格式: 0x19 0x06 [DTC_H] [DTC_M] [DTC_L] [ExtDataRecordNumber]
 *          响应格式: 0x59 0x06 [DTCFormat] [DTC_H] [DTC_M] [DTC_L] [Status] [ExtData...]
 */
UdsNrcCode UdsSvc19ReportDtcExtData(uint8_t *rxData, uint16_t rxLen, 
                                     uint8_t *txData, uint16_t *txLen)
{
    uint32_t dtcCode;
    uint8_t extDataRecordNum;
    UdsDtcExtData extData;
    uint16_t respIndex;
    UdsDtcEntry *entry;
    
    /* 检查请求长度: SID + SubFunction + DTC(3B) + ExtDataRecordNum */
    if (rxLen < 6u) {
        return UDS_NRC_INCORRECT_MESSAGE_LENGTH;
    }
    
    /* 解析DTC代码 */
    dtcCode = DtcDecode(&rxData[2]);
    
    /* 获取扩展数据记录号 */
    extDataRecordNum = rxData[5];
    
    /* 查找DTC条目 */
    entry = UdsDtcFindEntry(dtcCode);
    if (entry == NULL) {
        /* DTC未找到 - 按ISO 14229-1，应返回成功但状态为0 */
        /* 构建空响应 */
        respIndex = 0u;
        txData[respIndex++] = UDS_SVC19_RESP_DTC_LIST;  /* 0x59 */
        txData[respIndex++] = UDS_SVC19_REPORT_DTC_EXT_DATA;  /* 0x06 */
        txData[respIndex++] = UDS_DTC_FORMAT;            /* DTC格式 */
        DtcEncode(dtcCode, &txData[respIndex]);
        respIndex += 3u;
        txData[respIndex++] = 0u;  /* 状态字节为0 */
        /* 扩展数据记录号为0xFF时返回所有记录，否则返回指定记录 */
        if (extDataRecordNum == 0xFFu) {
            /* 返回所有扩展数据 */
            (void)memset(&txData[respIndex], 0u, UDS_DTC_EXT_DATA_SIZE);
            respIndex += UDS_DTC_EXT_DATA_SIZE;
        } else {
            /* 单个记录 - 简化处理：返回空 */
            txData[respIndex++] = extDataRecordNum;
        }
        
        *txLen = respIndex;
        return UDS_NRC_GENERAL_REJECT;
    }
    
    /* 获取扩展数据 */
    if (!UdsDtcGetExtData(dtcCode, &extData)) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 构建响应 */
    respIndex = 0u;
    txData[respIndex++] = UDS_SVC19_RESP_DTC_LIST;  /* 0x59 */
    txData[respIndex++] = UDS_SVC19_REPORT_DTC_EXT_DATA;  /* 0x06 */
    txData[respIndex++] = UDS_DTC_FORMAT;            /* DTC格式 */
    
    /* DTC代码 */
    DtcEncode(dtcCode, &txData[respIndex]);
    respIndex += 3u;
    
    /* 状态字节 */
    txData[respIndex++] = entry->status;
    
    /* 扩展数据记录号 */
    if (extDataRecordNum == 0xFFu) {
        /* 0xFF = 返回所有扩展数据记录 */
        /* 扩展数据内容 */
        txData[respIndex++] = extData.occurrenceCounter;
        txData[respIndex++] = extData.agingCounter;
        txData[respIndex++] = extData.faultDetectionCounter;
        txData[respIndex++] = 0u;  /* 保留 */
        txData[respIndex++] = 0u;  /* 保留 */
        txData[respIndex++] = 0u;  /* 保留 */
        txData[respIndex++] = 0u;  /* 保留 */
        txData[respIndex++] = 0u;  /* 保留 */
    } else {
        /* 单个记录 - 返回记录号 */
        txData[respIndex++] = extDataRecordNum;
        /* 简化处理：统一返回扩展数据 */
        txData[respIndex++] = extData.occurrenceCounter;
        txData[respIndex++] = extData.agingCounter;
        txData[respIndex++] = extData.faultDetectionCounter;
    }
    
    *txLen = respIndex;
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}
