/**
  * @file    uds_svc_10.h
  * @brief   诊断会话控制服务 (0x10) 接口
  * @details 定义0x10诊断会话控制服务的子服务和处理函数
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_SVC_10_H
#define UDS_SVC_10_H

#include <stdint.h>
#include "../uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 诊断会话控制子服务ID
 *===========================================================================*/
#define UDS_SVC_10_DEFAULT          0x01    /* 默认会话 */
#define UDS_SVC_10_PROGRAMMING      0x02    /* 编程会话 */
#define UDS_SVC_10_EXTENDED         0x03    /* 扩展诊断会话 */
#define UDS_SVC_10_SECURITY         0x04    /* 安全会话 */

/*=============================================================================
 * 0x10服务处理函数
 *===========================================================================*/

/**
  * @brief      默认会话处理 (0x1001)
  * @details    切换到默认会话，锁定安全访问，停止0x78等待
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       响应包含会话类型、P2Server、P2*Server参数
  * @warning    切换到默认会话会自动锁定所有安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10DefaultSession(uint8_t *rxData, uint16_t rxLen, 
                                   uint8_t *txData, uint16_t *txLen);

/**
  * @brief      编程会话处理 (0x1002)
  * @details    切换到编程会话，锁定安全访问
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       编程会话用于软件刷写
  * @warning    编程会话需要安全访问才能执行敏感操作
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10ProgrammingSession(uint8_t *rxData, uint16_t rxLen, 
                                       uint8_t *txData, uint16_t *txLen);

/**
  * @brief      扩展诊断会话处理 (0x1003)
  * @details    切换到扩展诊断会话
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       扩展会话允许执行更多诊断功能
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10ExtendedSession(uint8_t *rxData, uint16_t rxLen, 
                                    uint8_t *txData, uint16_t *txLen);

/**
  * @brief      安全会话处理 (0x1004)
  * @details    切换到安全会话
  * @param[in]  rxData 接收数据缓冲区
  * @param[in]  rxLen  接收数据长度
  * @param[out] txData 发送数据缓冲区
  * @param[out] txLen  发送数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       安全会话提供最高级别的诊断权限
  * @warning    安全会话通常需要先解锁安全访问
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsSvc10SecuritySession(uint8_t *rxData, uint16_t rxLen, 
                                    uint8_t *txData, uint16_t *txLen);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SVC_10_H */
