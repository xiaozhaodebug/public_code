/**
  * @file    uds_pending.c
  * @brief   UDS 0x78 ResponsePending 处理实现
  * @details 实现0x78响应等待机制，用于处理长耗时诊断服务
  * @author  [小昭debug]
  * @date    2026-03-09
  * @version 1.0.0
  * @copyright Copyright (c) 2026
  */

#include "uds_pending.h"
#include "uds_config.h"
#include "uds_types.h"

/*=============================================================================
 * 外部上下文声明
 *===========================================================================*/
extern UdsContext gUdsContext;

/**
  * @brief      启动0x78响应等待
  * @details    设置pending状态为激活，初始化计数和定时器
  * @param[in]  serviceId 正在处理的服务SID
  * @return     void
  * @note       启动后系统会定期发送0x78响应直到服务完成
  * @warning    必须先启动才能使用其他pending功能
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingStart(uint8_t serviceId)
{
    gUdsContext.pendingResp.active = true;
    gUdsContext.pendingResp.count = 0u;
    gUdsContext.pendingResp.timer = UDS_NRC_78_INTERVAL;
    gUdsContext.pendingResp.pendingService = serviceId;
}

/**
  * @brief      停止0x78响应等待
  * @details    清除pending状态和相关变量
  * @return     void
  * @note       服务完成或出错时必须调用
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingStop(void)
{
    gUdsContext.pendingResp.active = false;
    gUdsContext.pendingResp.count = 0u;
    gUdsContext.pendingResp.timer = 0u;
    gUdsContext.pendingResp.pendingService = 0u;
}

/**
  * @brief      检查是否正在发送0x78
  * @details    返回pending激活状态
  * @return     bool true表示正在发送
  * @note       用于判断当前是否处于响应等待模式
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingIsActive(void)
{
    return gUdsContext.pendingResp.active;
}

/**
  * @brief      获取已发送0x78的次数
  * @details    返回当前pending计数
  * @return     uint8_t 发送次数
  * @note       每次发送0x78后计数加1
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsPendingGetCount(void)
{
    return gUdsContext.pendingResp.count;
}

/**
  * @brief      0x78定时器处理
  * @details    递减pending间隔定时器
  * @return     void
  * @note       需要在1ms定时器中断中调用
  * @warning    定时器到期后需要主循环处理发送
  * @author     [小昭debug]
  * @date       2026-03-09
  */
void UdsPendingTimerHandler(void)
{
    if (!gUdsContext.pendingResp.active) {
        return;
    }
    
    if (gUdsContext.pendingResp.timer > 0u) {
        gUdsContext.pendingResp.timer--;
    }
}

/**
  * @brief      检查是否需要发送0x78
  * @details    检查pending激活状态和定时器
  * @return     bool true表示需要发送
  * @note       定时器为0且处于激活状态时返回true
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingNeedSend(void)
{
    if (!gUdsContext.pendingResp.active) {
        return false;
    }
    
    return (gUdsContext.pendingResp.timer == 0u);
}

/**
  * @brief      检查是否超过最大次数
  * @details    比较当前计数和配置的最大次数
  * @return     bool true表示超过最大次数
  * @note       超过后应发送最终否定响应并停止pending
  * @warning    超过最大次数意味着服务处理失败
  * @author     [小昭debug]
  * @date       2026-03-09
  */
bool UdsPendingIsMaxCountReached(void)
{
    if (!gUdsContext.pendingResp.active) {
        return false;
    }
    
    return (gUdsContext.pendingResp.count >= UDS_NRC_78_MAX_COUNT);
}

/**
  * @brief      获取正在处理的服务ID
  * @details    返回保存的服务SID
  * @return     uint8_t 服务SID
  * @note       用于构建0x78响应帧
  * @author     [小昭debug]
  * @date       2026-03-09
  */
uint8_t UdsPendingGetServiceId(void)
{
    return gUdsContext.pendingResp.pendingService;
}
