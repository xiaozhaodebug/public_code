/**
  * @file    uds_dtc.h
  * @brief   DTC管理模块头文件
  * @details 实现DTC的存储、状态管理、扩展数据记录等功能
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_DTC_H
#define UDS_DTC_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * DTC相关常量定义
 *===========================================================================*/
#define UDS_DTC_MAX_COUNT           32u     /* 最大支持的DTC数量 */
#define UDS_DTC_EXT_DATA_SIZE       8u      /* 扩展数据记录大小(字节) */

/* DTC状态掩码定义 (ISO 14229-1) */
#define UDS_DTC_STATUS_TEST_FAILED                  0x01u   /* bit0: 测试失败 */
#define UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE    0x02u   /* bit1: 本操作循环测试失败 */
#define UDS_DTC_STATUS_PENDING                      0x04u   /* bit2: 等待确认 */
#define UDS_DTC_STATUS_CONFIRMED                    0x08u   /* bit3: 已确认 */
#define UDS_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR  0x10u  /* bit4: 清除后测试未完成 */
#define UDS_DTC_STATUS_TEST_FAILED_SINCE_CLEAR      0x20u   /* bit5: 清除后测试失败过 */
#define UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE 0x40u /* bit6: 本循环测试未完成 */
#define UDS_DTC_STATUS_WARNING_INDICATOR            0x80u   /* bit7: 警告指示器请求 */

/* DTC格式类型 */
#define UDS_DTC_FORMAT_ISO11992     0x00u   /* ISO 11992-4 */
#define UDS_DTC_FORMAT_ISO14229     0x01u   /* ISO 14229-1 (3字节DTC) */
#define UDS_DTC_FORMAT_SAEJ1939     0x02u   /* SAE J1939 */
#define UDS_DTC_FORMAT_ISO13400     0x03u   /* ISO 13400-2 */

/*=============================================================================
 * DTC数据结构定义
 *===========================================================================*/

/**
 * @brief DTC扩展数据记录结构
 */
typedef struct {
    uint8_t occurrenceCounter;      /* 故障发生计数器 */
    uint8_t agingCounter;           /* 老化计数器 */
    uint8_t faultDetectionCounter;  /* 故障检测计数器 */
    uint8_t reserved[5];            /* 保留 */
} UdsDtcExtData;

/**
 * @brief DTC条目结构
 */
typedef struct {
    uint32_t dtcCode;               /* DTC代码 (3字节有效) */
    uint8_t  status;                /* DTC状态字节 */
    uint8_t  severity;              /* DTC严重程度 */
    bool     isValid;               /* 条目是否有效 */
    uint32_t firstDetectTime;       /* 首次检测时间戳 */
    uint32_t lastDetectTime;        /* 最后一次检测时间戳 */
    UdsDtcExtData extData;          /* 扩展数据记录 */
} UdsDtcEntry;

/*=============================================================================
 * DTC管理接口
 *===========================================================================*/

/**
 * @brief 初始化DTC管理模块
 */
void UdsDtcInit(void);

/**
 * @brief 报告/更新DTC状态
 * @param dtcCode DTC代码
 * @param testFailed true表示当前测试失败，false表示通过
 * @return true表示操作成功，false表示DTC表已满
 */
bool UdsDtcReport(uint32_t dtcCode, bool testFailed);

/**
 * @brief 清除所有DTC
 * @return true表示操作成功
 */
bool UdsDtcClearAll(void);

/**
 * @brief 根据状态掩码获取DTC列表
 * @param statusMask 状态掩码
 * @param[out] dtcList 输出DTC列表缓冲区
 * @param[in,out] count 输入：最大数量，输出：实际数量
 * @return 实际匹配的DTC数量
 */
uint8_t UdsDtcGetListByStatus(uint8_t statusMask, uint32_t *dtcList, uint8_t *count);

/**
 * @brief 获取所有支持的DTC列表
 * @param[out] dtcList 输出DTC列表缓冲区
 * @param[in,out] count 输入：最大数量，输出：实际数量
 * @return 实际DTC数量
 */
uint8_t UdsDtcGetSupportedList(uint32_t *dtcList, uint8_t *count);

/**
 * @brief 获取DTC的扩展数据记录
 * @param dtcCode DTC代码
 * @param[out] extData 扩展数据输出缓冲区
 * @return true表示找到DTC，false表示未找到
 */
bool UdsDtcGetExtData(uint32_t dtcCode, UdsDtcExtData *extData);

/**
 * @brief 根据DTC代码查找条目
 * @param dtcCode DTC代码
 * @return DTC条目指针，未找到返回NULL
 */
UdsDtcEntry* UdsDtcFindEntry(uint32_t dtcCode);

/**
 * @brief 获取DTC状态
 * @param dtcCode DTC代码
 * @return 状态字节，未找到返回0
 */
uint8_t UdsDtcGetStatus(uint32_t dtcCode);

/**
 * @brief 获取当前存储的DTC数量
 * @return DTC数量
 */
uint8_t UdsDtcGetCount(void);

/**
 * @brief DTC主任务处理（周期性调用）
 * @note 用于更新老化计数器等周期性操作
 */
void UdsDtcTask(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_DTC_H */
