/**
  * @file    uds_svc_19.h
  * @brief   读取DTC信息服务 (0x19) 接口
  * @details 实现0x19服务，支持读取故障码信息
  * @author  [小昭debug]
  * @date    2026-03-22
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_19_H
#define UDS_SVC_19_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 0x19服务子服务ID定义 (ISO 14229-1)
 *===========================================================================*/
#define UDS_SVC19_REPORT_NUM_DTC_BY_STATUS            0x01u  /* 报告DTC数量（按状态掩码） */
#define UDS_SVC19_REPORT_DTC_BY_STATUS                0x02u  /* 报告DTC列表（按状态掩码） */
#define UDS_SVC19_REPORT_DTC_SNAPSHOT                 0x04u  /* 报告DTC快照数据 */
#define UDS_SVC19_REPORT_DTC_EXT_DATA                 0x06u  /* 报告DTC扩展数据 */
#define UDS_SVC19_REPORT_NUM_DTC_BY_SEVERITY          0x07u  /* 报告DTC数量（按严重程度） */
#define UDS_SVC19_REPORT_DTC_BY_SEVERITY              0x08u  /* 报告DTC列表（按严重程度） */
#define UDS_SVC19_REPORT_SUPPORTED_DTC                0x0Au  /* 报告支持的DTC */
#define UDS_SVC19_REPORT_FIRST_TEST_FAILED_DTC        0x0Bu  /* 报告首次测试失败的DTC */
#define UDS_SVC19_REPORT_FIRST_CONFIRMED_DTC          0x0Cu  /* 报告首次确认的DTC */
#define UDS_SVC19_REPORT_MOST_RECENT_TEST_FAILED_DTC  0x0Du  /* 报告最近测试失败的DTC */
#define UDS_SVC19_REPORT_MOST_RECENT_CONFIRMED_DTC    0x0Eu  /* 报告最近确认的DTC */
#define UDS_SVC19_REPORT_MIRROR_MEMORY_DTC            0x0Fu  /* 报告镜像内存DTC */
#define UDS_SVC19_REPORT_MIRROR_MEMORY_EXT_DATA       0x10u  /* 报告镜像内存扩展数据 */
#define UDS_SVC19_REPORT_NUM_MIRROR_MEMORY_DTC        0x11u  /* 报告镜像内存DTC数量 */
#define UDS_SVC19_REPORT_NUM_EMISSIONS_DTC            0x12u  /* 报告排放相关DTC数量 */
#define UDS_SVC19_REPORT_CONFIRMED_DTC_EXT_DATA       0x13u  /* 报告确认DTC的扩展数据 */

/*=============================================================================
 * 0x19服务处理函数
 *===========================================================================*/

/**
 * @brief 0x19服务主处理函数
 * @param[in] rxData 接收数据缓冲区
 * @param[in] rxLen  接收数据长度
 * @param[out] txData 发送数据缓冲区
 * @param[out] txLen  发送数据长度指针
 * @return UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
 * @note 请求格式: 0x19 + SubFunction + [参数]
 * @note 响应格式: 0x59 + SubFunction + [DTC数据]
 */
UdsNrcCode UdsSvc19Handler(uint8_t *rxData, uint16_t rxLen, 
                            uint8_t *txData, uint16_t *txLen);

/**
 * @brief 0x19服务子服务：按状态掩码报告DTC列表 (0x02)
 */
UdsNrcCode UdsSvc19ReportDtcByStatus(uint8_t *rxData, uint16_t rxLen, 
                                      uint8_t *txData, uint16_t *txLen);

/**
 * @brief 0x19服务子服务：报告支持的DTC (0x0A)
 */
UdsNrcCode UdsSvc19ReportSupportedDtc(uint8_t *rxData, uint16_t rxLen, 
                                       uint8_t *txData, uint16_t *txLen);

/**
 * @brief 0x19服务子服务：报告DTC扩展数据 (0x06)
 */
UdsNrcCode UdsSvc19ReportDtcExtData(uint8_t *rxData, uint16_t rxLen, 
                                     uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_19_H */
