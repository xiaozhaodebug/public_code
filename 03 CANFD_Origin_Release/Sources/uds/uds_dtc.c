/**
  * @file    uds_dtc.c
  * @brief   DTC管理模块实现
  * @details 实现DTC的存储、状态管理、扩展数据记录等功能
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_dtc.h"
#include <string.h>

/*=============================================================================
 * 内部变量
 *===========================================================================*/
static UdsDtcEntry sDtcTable[UDS_DTC_MAX_COUNT];    /* DTC存储表 */
static uint8_t sDtcCount = 0;                        /* 当前DTC数量 */
static bool sDtcInitialized = false;                 /* 初始化标志 */

/*=============================================================================
 * 内部函数
 *===========================================================================*/

/**
 * @brief 查找或创建DTC条目
 * @param dtcCode DTC代码
 * @param createIfNotFound 如果不存在是否创建
 * @return DTC条目指针，失败返回NULL
 */
static UdsDtcEntry* DtcFindOrCreateEntry(uint32_t dtcCode, bool createIfNotFound)
{
    uint8_t i;
    UdsDtcEntry *emptySlot = NULL;
    
    /* 在现有条目中查找 */
    for (i = 0u; i < UDS_DTC_MAX_COUNT; i++) {
        if (sDtcTable[i].isValid && sDtcTable[i].dtcCode == dtcCode) {
            return &sDtcTable[i];
        }
        /* 记录第一个空槽位 */
        if (createIfNotFound && !sDtcTable[i].isValid && emptySlot == NULL) {
            emptySlot = &sDtcTable[i];
        }
    }
    
    /* 需要创建新条目 */
    if (createIfNotFound && emptySlot != NULL) {
        (void)memset(emptySlot, 0u, sizeof(UdsDtcEntry));
        emptySlot->dtcCode = dtcCode;
        emptySlot->isValid = true;
        emptySlot->status = UDS_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR |
                           UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE;
        sDtcCount++;
        return emptySlot;
    }
    
    return NULL;
}

/**
 * @brief 更新DTC状态
 * @param entry DTC条目
 * @param testFailed 当前测试结果
 */
static void DtcUpdateStatus(UdsDtcEntry *entry, bool testFailed)
{
    uint8_t newStatus = entry->status;
    
    if (testFailed) {
        /* 测试失败 - 设置相关位 */
        newStatus |= UDS_DTC_STATUS_TEST_FAILED;
        newStatus |= UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE;
        newStatus |= UDS_DTC_STATUS_PENDING;
        newStatus &= ~UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE;
        newStatus &= ~UDS_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR;
        newStatus |= UDS_DTC_STATUS_TEST_FAILED_SINCE_CLEAR;
        
        /* 如果已经连续多次检测到故障，设置为已确认状态 */
        if (entry->extData.occurrenceCounter >= 3u) {
            newStatus |= UDS_DTC_STATUS_CONFIRMED;
        }
    } else {
        /* 测试通过 - 清除TEST_FAILED位 */
        newStatus &= ~UDS_DTC_STATUS_TEST_FAILED;
        newStatus &= ~UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE;
        /* PENDING和CONFIRMED位不会立即清除 */
    }
    
    entry->status = newStatus;
}

/*=============================================================================
 * 外部接口实现
 *===========================================================================*/

/**
 * @brief 初始化DTC管理模块
 */
void UdsDtcInit(void)
{
    uint8_t i;
    
    if (sDtcInitialized) {
        return;
    }
    
    /* 清空DTC表 */
    (void)memset(sDtcTable, 0u, sizeof(sDtcTable));
    sDtcCount = 0u;
    
    /* 预设一些示例DTC（用于测试） */
    /* 注：这些预设DTC初始状态为无效，需要实际触发才会激活 */
    sDtcTable[0].dtcCode = 0xB0B0C00u;  /* 电池电压过高 */
    sDtcTable[1].dtcCode = 0xB056200u;  /* 系统电压低 */
    sDtcTable[2].dtcCode = 0xA010100u;  /* 质量或容积空气流量电路范围/性能 */
    sDtcTable[3].dtcCode = 0xC030000u;  /* 随机/多缸检测到失火 */
    
    for (i = 0u; i < 4u; i++) {
        sDtcTable[i].isValid = false;  /* 初始为无效，等待触发 */
    }
    
    sDtcInitialized = true;
}

/**
 * @brief 报告/更新DTC状态
 */
bool UdsDtcReport(uint32_t dtcCode, bool testFailed)
{
    UdsDtcEntry *entry;
    uint32_t currentTime;
    
    if (!sDtcInitialized) {
        UdsDtcInit();
    }
    
    /* 仅当测试失败时才创建新条目 */
    entry = DtcFindOrCreateEntry(dtcCode, testFailed);
    if (entry == NULL) {
        return false;
    }
    
    currentTime = 0u;  /* 简化处理，实际应使用系统时间 */
    
    /* 首次检测 */
    if (entry->firstDetectTime == 0u && testFailed) {
        entry->firstDetectTime = currentTime;
        entry->lastDetectTime = currentTime;
    }
    
    /* 更新状态 */
    DtcUpdateStatus(entry, testFailed);
    
    /* 更新计数器和扩展数据 */
    if (testFailed) {
        /* 增加发生计数器（最大255） */
        if (entry->extData.occurrenceCounter < 0xFFu) {
            entry->extData.occurrenceCounter++;
        }
        entry->lastDetectTime = currentTime;
    } else {
        /* 测试通过时，如果confirmed位已设置，增加老化计数器 */
        if ((entry->status & UDS_DTC_STATUS_CONFIRMED) != 0u) {
            if (entry->extData.agingCounter < 0xFFu) {
                entry->extData.agingCounter++;
            }
            /* 老化计数器达到阈值后清除confirmed状态 */
            if (entry->extData.agingCounter >= 40u) {
                entry->status &= ~UDS_DTC_STATUS_CONFIRMED;
            }
        }
    }
    
    return true;
}

/**
 * @brief 清除所有DTC
 */
bool UdsDtcClearAll(void)
{
    uint8_t i;
    
    if (!sDtcInitialized) {
        return true;
    }
    
    for (i = 0u; i < UDS_DTC_MAX_COUNT; i++) {
        if (sDtcTable[i].isValid) {
            sDtcTable[i].isValid = false;
        }
    }
    sDtcCount = 0u;
    
    return true;
}

/**
 * @brief 根据状态掩码获取DTC列表
 */
uint8_t UdsDtcGetListByStatus(uint8_t statusMask, uint32_t *dtcList, uint8_t *count)
{
    uint8_t i;
    uint8_t matched = 0u;
    uint8_t maxCount;
    
    if (dtcList == NULL || count == NULL) {
        return 0u;
    }
    
    maxCount = *count;
    if (maxCount == 0u) {
        return 0u;
    }
    
    if (!sDtcInitialized) {
        *count = 0u;
        return 0u;
    }
    
    for (i = 0u; i < UDS_DTC_MAX_COUNT && matched < maxCount; i++) {
        if (sDtcTable[i].isValid) {
            /* 检查状态掩码匹配 (按位与) */
            if ((sDtcTable[i].status & statusMask) != 0u) {
                dtcList[matched++] = sDtcTable[i].dtcCode;
            }
        }
    }
    
    *count = matched;
    return matched;
}

/**
 * @brief 获取所有支持的DTC列表
 */
uint8_t UdsDtcGetSupportedList(uint32_t *dtcList, uint8_t *count)
{
    uint8_t i;
    uint8_t matched = 0u;
    uint8_t maxCount;
    
    if (dtcList == NULL || count == NULL) {
        return 0u;
    }
    
    maxCount = *count;
    if (maxCount == 0u) {
        return 0u;
    }
    
    if (!sDtcInitialized) {
        *count = 0u;
        return 0u;
    }
    
    /* 返回所有有效DTC */
    for (i = 0u; i < UDS_DTC_MAX_COUNT && matched < maxCount; i++) {
        if (sDtcTable[i].isValid) {
            dtcList[matched++] = sDtcTable[i].dtcCode;
        }
    }
    
    *count = matched;
    return matched;
}

/**
 * @brief 获取DTC的扩展数据记录
 */
bool UdsDtcGetExtData(uint32_t dtcCode, UdsDtcExtData *extData)
{
    UdsDtcEntry *entry;
    
    if (extData == NULL || !sDtcInitialized) {
        return false;
    }
    
    entry = UdsDtcFindEntry(dtcCode);
    if (entry == NULL) {
        return false;
    }
    
    (void)memcpy(extData, &entry->extData, sizeof(UdsDtcExtData));
    return true;
}

/**
 * @brief 根据DTC代码查找条目
 */
UdsDtcEntry* UdsDtcFindEntry(uint32_t dtcCode)
{
    uint8_t i;
    
    if (!sDtcInitialized) {
        return NULL;
    }
    
    for (i = 0u; i < UDS_DTC_MAX_COUNT; i++) {
        if (sDtcTable[i].isValid && sDtcTable[i].dtcCode == dtcCode) {
            return &sDtcTable[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 获取DTC状态
 */
uint8_t UdsDtcGetStatus(uint32_t dtcCode)
{
    UdsDtcEntry *entry = UdsDtcFindEntry(dtcCode);
    if (entry != NULL) {
        return entry->status;
    }
    return 0u;
}

/**
 * @brief 获取当前存储的DTC数量
 */
uint8_t UdsDtcGetCount(void)
{
    return sDtcCount;
}

/**
 * @brief DTC主任务处理（周期性调用）
 */
void UdsDtcTask(void)
{
    /* 可以在这里实现周期性任务，如老化处理等 */
    /* 简化实现：暂无周期性操作 */
}
