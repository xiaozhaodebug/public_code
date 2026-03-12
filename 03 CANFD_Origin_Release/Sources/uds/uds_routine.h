/**
  * @file    uds_routine.h
  * @brief   UDS例程控制管理接口 (0x31服务)
  * @details 定义例程控制服务接口，支持例程启动、停止、请求结果
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#ifndef UDS_ROUTINE_H
#define UDS_ROUTINE_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 例程控制接口函数
 *===========================================================================*/

/**
  * @brief      例程控制配置初始化
  * @details    设置例程配置表和例程数量
  * @param[in]  routineTable 例程配置表指针
  * @param[in]  routineCount 例程数量
  * @return     void
  * @note       配置表包含RID、权限掩码、回调函数等
  * @warning    配置表指针必须在整个生命周期内有效
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsRoutineConfigInit(const UdsRoutineConfig *routineTable, uint8_t routineCount);

/**
  * @brief      查找例程配置
  * @details    根据RID在配置表中查找对应的例程配置
  * @param[in]  rid 例程标识符(2字节)
  * @return     const UdsRoutineConfig* 配置指针，未找到返回NULL
  * @note       RID按大端字节序解析
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const UdsRoutineConfig* UdsRoutineFind(uint16_t rid);

/**
  * @brief      检查例程权限
  * @details    检查当前会话和安全状态是否满足例程执行要求
  * @param[in]  config 例程配置指针
  * @return     bool   true表示有权限执行
  * @note       同时检查会话权限和安全访问权限
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsRoutineCheckPermission(const UdsRoutineConfig *config);

/**
  * @brief      执行例程启动
  * @details    调用例程的启动回调函数
  * @param[in]  config  例程配置指针
  * @param[in]  data    请求数据（选项记录）
  * @param[in]  len     请求数据长度
  * @param[out] resp    响应数据缓冲区
  * @param[out] respLen 响应数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       启动回调函数由用户实现
  * @warning    确保resp缓冲区足够大以存储响应数据
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteStart(const UdsRoutineConfig *config, 
                                   uint8_t *data, uint16_t len, 
                                   uint8_t *resp, uint16_t *respLen);

/**
  * @brief      执行例程停止
  * @details    调用例程的停止回调函数
  * @param[in]  config  例程配置指针
  * @param[in]  data    请求数据（选项记录）
  * @param[in]  len     请求数据长度
  * @param[out] resp    响应数据缓冲区
  * @param[out] respLen 响应数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       停止回调函数由用户实现
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteStop(const UdsRoutineConfig *config, 
                                  uint8_t *data, uint16_t len, 
                                  uint8_t *resp, uint16_t *respLen);

/**
  * @brief      执行例程请求结果
  * @details    调用例程的请求结果回调函数
  * @param[in]  config  例程配置指针
  * @param[in]  data    请求数据（选项记录）
  * @param[in]  len     请求数据长度
  * @param[out] resp    响应数据缓冲区
  * @param[out] respLen 响应数据长度指针
  * @return     UdsNrcCode NRC码，UDS_NRC_GENERAL_REJECT表示成功
  * @note       请求结果回调函数由用户实现
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteRequest(const UdsRoutineConfig *config, 
                                     uint8_t *data, uint16_t len, 
                                     uint8_t *resp, uint16_t *respLen);

/**
  * @brief      检查例程是否正在运行
  * @details    调用例程的状态查询回调函数
  * @param[in]  config 例程配置指针
  * @return     bool   true表示正在运行
  * @note       状态回调函数由用户实现
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsRoutineIsRunning(const UdsRoutineConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* UDS_ROUTINE_H */
