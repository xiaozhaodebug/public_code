/**
 * @file uds_user_config_example.c
 * @brief UDS用户配置示例
 * @note 用户需根据实际情况修改此文件
 */

#include "uds_stack.h"
#include "can_pal.h"
#include "can_fd_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include "Cpu.h"
#include "services/uds_svc_22.h"
#include "services/uds_svc_19.h"
#include "services/uds_svc_2e.h"
#include "uds_dtc.h"
/*=============================================================================
 * 平台层实现
 *===========================================================================*/

/**
  * @brief      CAN发送实现
  * @details    调用CanFdSendQuick发送CAN报文，支持数据填充到配置长度
  * @param[in]  canId  CAN ID
  * @param[in]  data   数据指针
  * @param[in]  length 数据长度
  * @return     status_t 发送状态
  * @note       若使能UDS_TX_PADDING_ENABLE，不足UDS_TX_PADDING_LENGTH的字节将填充UDS_TX_PADDING_BYTE
  * @warning    填充功能需要本地缓冲区，最大支持64字节
  * @author     [小昭debug]
  * @date       2026-03-09
  */
status_t UdsPlatformCanTransmit(uint32_t canId, uint8_t *data, uint8_t length)
{
#if (UDS_TX_PADDING_ENABLE == 1)
    uint8_t txBuffer[64];
    uint8_t sendLength;
    
    /* 参数检查 */
    if (data == NULL || length == 0u) {
        return STATUS_ERROR;
    }
    
    /* 限制最大长度 */
    if (length > UDS_TX_PADDING_LENGTH) {
        sendLength = (uint8_t)UDS_TX_PADDING_LENGTH;
    } else {
        sendLength = length;
    }
    
    /* 复制实际数据 */
    (void)memcpy(txBuffer, data, sendLength);
    
    /* 填充剩余字节 */
    if (sendLength < UDS_TX_PADDING_LENGTH) {
        (void)memset(&txBuffer[sendLength], UDS_TX_PADDING_BYTE, 
                     (size_t)(UDS_TX_PADDING_LENGTH - sendLength));
    }
    
    /* 使用填充后的长度发送 */
    return CanFdSendQuick(&can_pal0_instance, TX_MAILBOX_CAN0, canId, 
                          txBuffer, (uint8_t)UDS_TX_PADDING_LENGTH);
#else
    /* 不使能填充：直接发送原始数据 */
    return CanFdSendQuick(&can_pal0_instance, TX_MAILBOX_CAN0, canId, data, length);
#endif
}

/**
 * @brief 随机数生成实现
 * @note 实际项目中应使用安全的随机数生成器
 */
uint32_t UdsPlatformRandomGenerate(void)
{
    /* 示例：使用简单随机数，实际应用应使用硬件随机数或加密安全随机数 */
    static uint32_t seed = 0;
    if (seed == 0) {
        seed = (uint32_t)UdsPlatformGetTimeMs();
    }
    seed = (seed * 1103515245u + 12345u) & 0x7FFFFFFFu;
    return seed;
}

/**
 * @brief 获取当前时间戳
 */
uint32_t UdsPlatformGetTimeMs(void)
{
    /* 用户实现：返回系统运行的毫秒数 */
    extern uint32_t tickMs;
    return tickMs;
}

/**
 * @brief 系统复位
 */
void UdsPlatformSystemReset(uint8_t resetType)
{
    (void)resetType;
    /* 用户实现：执行系统复位 */
    /* NVIC_SystemReset(); */
}

/*=============================================================================
 * 0x27安全访问算法实现
 *===========================================================================*/

/**
 * @brief 安全等级1密钥计算
 * @param seed 4字节种子
 * @return 4字节密钥
 * @note 用户需实现自己的安全算法
 */
uint32_t MySecurityLevel1CalcKey(uint32_t seed)
{
    /* 示例算法：种子异或固定值 */
    /* 实际应用应使用更复杂的算法 */
    return seed ^ 0x12345678u;
}

/* 安全访问等级配置 */
const UdsSecurityLevelConfig gUdsSecurityConfig[] = {
    {
        1,                      /* 安全等级1 */
        MySecurityLevel1CalcKey, /* 密钥计算函数 */
        3,                      /* 最大尝试次数 */
        10000                   /* 锁定延时10秒 (ms) */
    }
};

/*=============================================================================
 * 0x31例程控制回调实现（示例）
 *===========================================================================*/

/**
 * @brief 示例：擦除内存例程 - 启动
 */
UdsNrcCode RoutineEraseMemoryStart(uint8_t *data, uint16_t len, 
                                    uint8_t *resp, uint16_t *respLen)
{
    (void)data;
    (void)len;
    
    /* 用户实现：启动内存擦除 */
    
    /* 构建响应 */
    resp[0] = 0x71;
    resp[1] = 0x01;  /* routineType */
    resp[2] = 0x02;  /* RID High */
    resp[3] = 0x01;  /* RID Low */
    resp[4] = 0x00;  /* 状态：进行中 */
    *respLen = 5;
    
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}

/**
 * @brief 示例：擦除内存例程 - 请求结果
 */
UdsNrcCode RoutineEraseMemoryResult(uint8_t *data, uint16_t len, 
                                     uint8_t *resp, uint16_t *respLen)
{
    (void)data;
    (void)len;
    
    /* 用户实现：查询擦除状态 */
    
    /* 构建响应 */
    resp[0] = 0x71;
    resp[1] = 0x03;  /* routineType */
    resp[2] = 0x02;  /* RID High */
    resp[3] = 0x01;  /* RID Low */
    resp[4] = 0x00;  /* 状态：完成 */
    *respLen = 5;
    
    return UDS_NRC_GENERAL_REJECT;  /* 成功 */
}

/* 例程配置表 */
const UdsRoutineConfig gUdsRoutineTable[] = {
    {
        0x0201,                                     /* RID: 擦除内存 */
        (1u << UDS_SESSION_PROGRAMMING),            /* 仅在编程会话可用 */
        true,                                       /* 需要安全访问 */
        1,                                          /* 安全等级1 */
        RoutineEraseMemoryStart,                    /* 启动回调 */
        NULL,                                       /* 停止回调 */
        RoutineEraseMemoryResult,                   /* 请求结果回调 */
        NULL                                        /* 状态查询回调 */
    }
};

/*=============================================================================
 * 0x10服务子服务配置
 *===========================================================================*/
const UdsSubServiceConfig gUdsSvc10SubTable[] = {
    {0x01, UdsSvc10DefaultSession,     (1u << UDS_SESSION_DEFAULT),                                    false, 0},
    {0x02, UdsSvc10ProgrammingSession, (1u << UDS_SESSION_DEFAULT) | (1u << UDS_SESSION_EXTENDED),     false, 0},
    {0x03, UdsSvc10ExtendedSession,    (1u << UDS_SESSION_DEFAULT) | (1u << UDS_SESSION_EXTENDED),     false, 0},
    {0x04, UdsSvc10SecuritySession,    (1u << UDS_SESSION_EXTENDED),                                   true,  1},
};

/*=============================================================================
 * 0x27服务子服务配置
 *===========================================================================*/
const UdsSubServiceConfig gUdsSvc27SubTable[] = {
    {0x01, UdsSvc27RequestSeed,  (1u << UDS_SESSION_EXTENDED) | (1u << UDS_SESSION_SECURITY),    false, 0},
    {0x02, UdsSvc27SendKey,      (1u << UDS_SESSION_EXTENDED) | (1u << UDS_SESSION_SECURITY),    false, 0},
};

/*=============================================================================
 * 主服务配置表
 *===========================================================================*/
const UdsServiceConfig gUdsServiceTable[] = {
    {0x10, NULL,                gUdsSvc10SubTable,  sizeof(gUdsSvc10SubTable) / sizeof(gUdsSvc10SubTable[0])},
    {0x19, UdsSvc19Handler,     NULL,               0},  /* 0x19读取DTC信息 */
    {0x22, UdsSvc22Handler,     NULL,               0},  /* 0x22根据标识符读取数据 */
    {0x27, NULL,                gUdsSvc27SubTable,  sizeof(gUdsSvc27SubTable) / sizeof(gUdsSvc27SubTable[0])},
    {0x2E, UdsSvc2eHandler,     NULL,               0},  /* 0x2E根据标识符写入数据（用于DTC故障注入） */
    {0x31, UdsSvc31Handler,     NULL,               0},  /* 0x31使用自定义处理 */
};

/*=============================================================================
 * UDS初始化函数（用户调用）
 *===========================================================================*/
void UdsStackInit(void)
{
    /* 初始化UDS协议栈 */
    UdsInit(gUdsServiceTable, 
            sizeof(gUdsServiceTable) / sizeof(gUdsServiceTable[0]));
    
    /* 配置安全访问 */
    UdsSecurityConfigInit(gUdsSecurityConfig);
    
    /* 配置例程控制 */
    UdsRoutineConfigInit(gUdsRoutineTable,
                         sizeof(gUdsRoutineTable) / sizeof(gUdsRoutineTable[0]));
    
    /* 初始化DTC管理模块 */
    UdsDtcInit();
}

/*=============================================================================
 * CAN接收回调集成示例
 *===========================================================================*/
/*
void CAN0_Callback_Func(uint32_t instance, can_event_t event, 
                        uint32_t buffIdx, void *flexcanState)
{
    (void)flexcanState;
    (void)instance;
    
    if (event == CAN_EVENT_RX_COMPLETE) {
        can_message_t rxMsg;
        CAN_Receive(&can_pal0_instance, RX_MAILBOX_CAN0, &rxMsg);
        
        // 调用UDS接收指示
        UdsCanRxIndication(rxMsg.id, rxMsg.data, rxMsg.length);
    }
}
*/

/*=============================================================================
 * 定时器中断集成示例
 *===========================================================================*/
/*
void LPIT_ISR(void)
{
    // 调用UDS定时器处理
    UdsTimerISR();
}
*/

/*=============================================================================
 * 主循环集成示例
 *===========================================================================*/
/*
int main(void)
{
    // 系统初始化...
    
    // 初始化UDS协议栈
    UdsStackInit();
    
    while (1) {
        // UDS主任务处理
        UdsMainTask();
        
        // 其他任务...
    }
}
*/
