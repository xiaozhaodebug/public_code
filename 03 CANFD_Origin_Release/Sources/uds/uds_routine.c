/**
  * @file    uds_routine.c
  * @brief   UDS例程控制管理实现
  * @details 实现例程控制服务，包括例程查找、权限检查、执行控制
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_routine.h"
#include "uds_session.h"
#include "uds_security.h"
#include <stddef.h>

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      例程控制配置初始化
  * @details    保存例程配置表指针和例程数量
  * @param[in]  routineTable 例程配置表
  * @param[in]  routineCount 例程数量
  * @return     void
  * @note       配置表定义了支持的RID及其处理函数
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsRoutineConfigInit(const UdsRoutineConfig *routineTable, uint8_t routineCount)
{
    gUdsContext.routineTable = routineTable;
    gUdsContext.routineCount = routineCount;
}

/**
  * @brief      查找例程配置
  * @details    遍历例程配置表查找指定的RID
  * @param[in]  rid 例程标识符
  * @return     const UdsRoutineConfig* 配置指针，未找到返回NULL
  * @note       线性查找，配置表应按RID排序以优化查找速度
  * @author     [小昭debug]
  * @date       2026-03-09
  */
const UdsRoutineConfig* UdsRoutineFind(uint16_t rid)
{
    const UdsRoutineConfig *config;
    uint8_t i;
    
    if (gUdsContext.routineTable == NULL || gUdsContext.routineCount == 0u) {
        return NULL;
    }
    
    config = gUdsContext.routineTable;
    
    for (i = 0u; i < gUdsContext.routineCount; i++) {
        if (config->rid == rid) {
            return config;
        }
        config++;
    }
    
    return NULL;
}

/**
  * @brief      检查例程权限
  * @details    检查当前会话和安全状态是否满足例程执行要求
  * @param[in]  config 例程配置指针
  * @return     bool   true表示有权限
  * @note       先检查会话权限，再检查安全访问权限
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsRoutineCheckPermission(const UdsRoutineConfig *config)
{
    if (config == NULL) {
        return false;
    }
    
    /* 检查会话权限 */
    if (!UdsCheckSessionPermission(config->sessionMask)) {
        return false;
    }
    
    /* 检查安全访问权限 */
    if (config->needSecurity) {
        if (!UdsCheckSecurityPermission(config->securityLevel)) {
            return false;
        }
    }
    
    return true;
}

/**
  * @brief      执行例程启动
  * @details    调用配置表中注册的启动回调函数
  * @param[in]  config  例程配置
  * @param[in]  data    请求数据
  * @param[in]  len     数据长度
  * @param[out] resp    响应数据
  * @param[out] respLen 响应长度
  * @return     UdsNrcCode NRC码
  * @note       如果未注册启动回调，返回SUB_FUNCTION_NOT_SUPPORTED
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteStart(const UdsRoutineConfig *config, 
                                   uint8_t *data, uint16_t len, 
                                   uint8_t *resp, uint16_t *respLen)
{
    if (config == NULL) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 检查是否有启动回调 */
    if (config->routineStart == NULL) {
        return UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
    }
    
    /* 调用用户回调 */
    return config->routineStart(data, len, resp, respLen);
}

/**
  * @brief      执行例程停止
  * @details    调用配置表中注册的停止回调函数
  * @param[in]  config  例程配置
  * @param[in]  data    请求数据
  * @param[in]  len     数据长度
  * @param[out] resp    响应数据
  * @param[out] respLen 响应长度
  * @return     UdsNrcCode NRC码
  * @note       如果未注册停止回调，返回SUB_FUNCTION_NOT_SUPPORTED
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteStop(const UdsRoutineConfig *config, 
                                  uint8_t *data, uint16_t len, 
                                  uint8_t *resp, uint16_t *respLen)
{
    if (config == NULL) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 检查是否有停止回调 */
    if (config->routineStop == NULL) {
        return UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
    }
    
    /* 调用用户回调 */
    return config->routineStop(data, len, resp, respLen);
}

/**
  * @brief      执行例程请求结果
  * @details    调用配置表中注册的请求结果回调函数
  * @param[in]  config  例程配置
  * @param[in]  data    请求数据
  * @param[in]  len     数据长度
  * @param[out] resp    响应数据
  * @param[out] respLen 响应长度
  * @return     UdsNrcCode NRC码
  * @note       如果未注册请求结果回调，返回SUB_FUNCTION_NOT_SUPPORTED
  * @author     [小昭debug]
  * @date       2026-03-09
  */
UdsNrcCode UdsRoutineExecuteRequest(const UdsRoutineConfig *config, 
                                     uint8_t *data, uint16_t len, 
                                     uint8_t *resp, uint16_t *respLen)
{
    if (config == NULL) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    /* 检查是否有请求结果回调 */
    if (config->routineRequest == NULL) {
        return UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED;
    }
    
    /* 调用用户回调 */
    return config->routineRequest(data, len, resp, respLen);
}

/**
  * @brief      检查例程是否正在运行
  * @details    调用配置表中注册的状态查询回调函数
  * @param[in]  config 例程配置
  * @return     bool   true表示正在运行
  * @note       如果未注册状态回调，返回false
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsRoutineIsRunning(const UdsRoutineConfig *config)
{
    if (config == NULL) {
        return false;
    }
    
    if (config->isRunning != NULL) {
        return config->isRunning();
    }
    
    return false;
}
