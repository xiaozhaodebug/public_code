/**
 * @file uds_types.h
 * @brief UDS协议栈类型定义
 */

#ifndef UDS_TYPES_H
#define UDS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 会话类型定义
 *===========================================================================*/
typedef enum {
    UDS_SESSION_DEFAULT         = 0x01,   /* 默认会话 */
    UDS_SESSION_PROGRAMMING     = 0x02,   /* 编程会话 */
    UDS_SESSION_EXTENDED        = 0x03,   /* 扩展诊断会话 */
    UDS_SESSION_SECURITY        = 0x04    /* 安全会话 */
} UdsSessionType;

/*=============================================================================
 * 安全访问状态
 *===========================================================================*/
typedef enum {
    UDS_SECURITY_LOCKED     = 0x00,   /* 锁定状态 */
    UDS_SECURITY_UNLOCKED   = 0x01,   /* 解锁状态 */
    UDS_SECURITY_WAIT_KEY   = 0x02    /* 等待密钥 */
} UdsSecurityState;

/*=============================================================================
 * 否定响应码 (NRC) - ISO 14229-1
 *===========================================================================*/
typedef enum {
    UDS_NRC_GENERAL_REJECT                  = 0x10,
    UDS_NRC_SERVICE_NOT_SUPPORTED           = 0x11,
    UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED      = 0x12,
    UDS_NRC_INCORRECT_MESSAGE_LENGTH        = 0x13,
    UDS_NRC_CONDITIONS_NOT_CORRECT          = 0x22,
    UDS_NRC_REQUEST_SEQUENCE_ERROR          = 0x24,
    UDS_NRC_REQUEST_OUT_OF_RANGE            = 0x31,
    UDS_NRC_SECURITY_ACCESS_DENIED          = 0x33,
    UDS_NRC_INVALID_KEY                     = 0x35,
    UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS       = 0x36,
    UDS_NRC_REQUIRED_TIME_DELAY             = 0x37,
    UDS_NRC_RESPONSE_PENDING                = 0x78,
    UDS_NRC_SUB_FUNC_NOT_SUPPORT_SESSION    = 0x7E,
    UDS_NRC_SERVICE_NOT_SUPPORT_SESSION     = 0x7F
} UdsNrcCode;

/*=============================================================================
 * TP层帧类型 (ISO 15765-2)
 *===========================================================================*/
typedef enum {
    UDS_TP_SF = 0,    /* 单帧 (Single Frame) */
    UDS_TP_FF,        /* 首帧 (First Frame) */
    UDS_TP_CF,        /* 连续帧 (Consecutive Frame) */
    UDS_TP_FC         /* 流控帧 (Flow Control) */
} UdsTpFrameType;

/*=============================================================================
 * 寻址类型
 *===========================================================================*/
typedef enum {
    UDS_ADDR_PHYSICAL   = 0,   /* 物理寻址 */
    UDS_ADDR_FUNCTIONAL = 1    /* 功能寻址 */
} UdsAddrType;

/*=============================================================================
 * 例程控制类型
 *===========================================================================*/
typedef enum {
    UDS_ROUTINE_START   = 0x01,   /* 启动例程 */
    UDS_ROUTINE_STOP    = 0x02,   /* 停止例程 */
    UDS_ROUTINE_REQUEST = 0x03    /* 请求结果 */
} UdsRoutineType;

/*=============================================================================
 * 流控帧状态
 *===========================================================================*/
typedef enum {
    UDS_FC_CTS   = 0,      /* Continue To Send - 继续发送 */
    UDS_FC_WAIT  = 1,      /* Wait - 等待 */
    UDS_FC_OVFLW = 2       /* Overflow - 缓冲区溢出 */
} UdsFcStatus;

/*=============================================================================
 * 前向声明
 *===========================================================================*/
struct UdsContext;
struct UdsSubServiceConfig;

/*=============================================================================
 * 服务处理函数类型
 *===========================================================================*/
typedef UdsNrcCode (*UdsServiceHandler)(
    uint8_t *rxData, 
    uint16_t rxLen, 
    uint8_t *txData, 
    uint16_t *txLen
);

/*=============================================================================
 * 子服务配置结构体
 *===========================================================================*/
typedef struct UdsSubServiceConfig {
    uint8_t             subServiceId;       /* 子服务ID */
    UdsServiceHandler   handler;            /* 处理函数 */
    uint32_t            sessionMask;        /* 允许会话的位图 */
    bool                needSecurity;       /* 是否需要安全访问 */
    uint8_t             securityLevel;      /* 所需安全等级 */
} UdsSubServiceConfig;

/*=============================================================================
 * 主服务配置结构体
 *===========================================================================*/
typedef struct {
    uint8_t                         serviceId;          /* 服务SID */
    UdsServiceHandler               defaultHandler;     /* 默认处理函数 */
    const UdsSubServiceConfig       *subServiceTable;   /* 子服务表 */
    uint8_t                         subServiceCount;    /* 子服务数量 */
} UdsServiceConfig;

/*=============================================================================
 * 密钥计算函数类型
 *===========================================================================*/
typedef uint32_t (*UdsSecurityCalcKeyFunc)(uint32_t seed);

/*=============================================================================
 * 安全访问等级配置
 *===========================================================================*/
typedef struct {
    uint8_t                     level;          /* 安全等级 */
    UdsSecurityCalcKeyFunc      calcKeyFunc;    /* 密钥计算函数指针 */
    uint8_t                     maxAttempts;    /* 最大尝试次数 */
    uint32_t                    delayTime;      /* 锁定后延时(ms) */
} UdsSecurityLevelConfig;

/*=============================================================================
 * 例程控制配置结构体
 *===========================================================================*/
typedef struct {
    uint16_t            rid;            /* 例程标识符 (2字节) */
    uint32_t            sessionMask;    /* 支持的会话位图 */
    bool                needSecurity;   /* 是否需要安全访问 */
    uint8_t             securityLevel;  /* 所需安全等级 */
    /* 回调函数 - 分别处理启动/停止/请求结果 */
    UdsNrcCode (*routineStart)(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);
    UdsNrcCode (*routineStop)(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);
    UdsNrcCode (*routineRequest)(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);
    /* 可选: 例程状态查询 */
    bool (*isRunning)(void);
} UdsRoutineConfig;

/*=============================================================================
 * TP层状态结构体
 *===========================================================================*/
typedef struct {
    /* 接收状态 */
    bool                rxInProgress;
    uint8_t             rxBuffer[UDS_TP_MAX_FRAME_SIZE];
    uint16_t            rxLength;
    uint16_t            rxIndex;
    uint8_t             rxExpectedSeq;
    uint16_t            rxBlockSize;
    uint32_t            rxTimer;
    
    /* 发送状态 */
    bool                txInProgress;
    uint8_t             txBuffer[UDS_TP_MAX_FRAME_SIZE];
    uint16_t            txLength;
    uint16_t            txIndex;
    uint8_t             txSeq;
    uint8_t             txBlockSize;
    uint8_t             txSTmin;
    uint32_t            txTimer;
    uint32_t            txFcWaitTimer;
    
    /* 流控状态 */
    bool                fcWait;
    uint8_t             fcStatus;       /* 0=继续, 1=等待, 2=溢出 */
} UdsTpContext;

/*=============================================================================
 * 0x78 ResponsePending 状态
 *===========================================================================*/
typedef struct {
    bool                active;         /* 是否正在发送0x78 */
    uint8_t             count;          /* 已发送次数 */
    uint32_t            timer;          /* 发送间隔定时器 */
    uint8_t             pendingService; /* 正在处理的服务SID */
} UdsPendingResponse;

/*=============================================================================
 * UDS全局上下文
 *===========================================================================*/
typedef struct UdsContext {
    /* 寻址信息 */
    UdsAddrType                 addrType;
    bool                        suppressPosResp;    /* 抑制肯定响应位 */
    
    /* 会话管理 */
    UdsSessionType              currentSession;
    uint32_t                    s3Timer;
    bool                        s3Active;
    
    /* 安全访问 */
    UdsSecurityState            securityState;
    uint8_t                     securityLevel;
    uint8_t                     securitySeed[UDS_SECURITY_SEED_SIZE];
    uint8_t                     failedAttempts;
    uint32_t                    securityDelayTimer;
    const UdsSecurityLevelConfig *securityConfig;
    
    /* 时间参数 */
    uint32_t                    p2Timer;
    bool                        p2Active;
    bool                        p2Extended;
    
    /* 0x78 ResponsePending */
    UdsPendingResponse          pendingResp;
    
    /* TP层上下文 */
    UdsTpContext                tpContext;
    
    /* 服务配置表 */
    const UdsServiceConfig      *serviceTable;
    uint8_t                     serviceCount;
    
    /* 例程配置表 */
    const UdsRoutineConfig      *routineTable;
    uint8_t                     routineCount;
} UdsContext;

#ifdef __cplusplus
}
#endif

#endif /* UDS_TYPES_H */
