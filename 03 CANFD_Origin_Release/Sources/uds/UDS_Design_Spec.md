# UDS 协议栈设计规格书

## 1. 概述

### 1.1 设计目标
- 基于 ISO 14229-1 (UDS) 和 ISO 15765-2 (CAN TP) 标准
- 兼容经典 CAN (8字节) 和 CAN FD (64字节)
- 支持多帧传输 (最大 4095 字节)
- 配置表驱动架构，便于维护和扩展

### 1.2 编码规范
- 采用 **驼峰命名风格** (CamelCase)
- 宏定义使用全大写 + 下划线
- 函数、变量使用驼峰命名

---

## 2. 硬件/底层接口

### 2.1 CAN ID 配置
| 类型 | ID (Hex) | 说明 |
|------|----------|------|
| 物理请求地址 | 0x74C | ECU接收诊断请求 |
| 物理响应地址 | 0x75C | ECU发送诊断响应 |
| 功能寻址 | 0x7DF | 广播请求 |

### 2.2 底层接口函数

```c
// CAN接收回调 - 由用户在中断中调用
void UdsCanRxIndication(uint32_t canId, uint8_t *data, uint8_t length);

// CAN发送接口 - 调用用户提供的 CanFdSendQuick
status_t UdsCanTransmit(uint32_t canId, uint8_t *data, uint8_t length);

// 1ms定时器中断 - 由用户在 LPIT_ISR 中调用
void UdsTimerISR(void);
```

---

## 3. 时间参数定义

### 3.1 CAN TP 层时间参数 (ISO 15765-2)
```c
#define UDS_TP_N_AR          1000u   /* 接收方响应时间 (ms) */
#define UDS_TP_N_BR          1000u   /* 接收方等待时间 (ms) */
#define UDS_TP_N_CR          1000u   /* 接收方连续帧等待 (ms) */
#define UDS_TP_N_AS          1000u   /* 发送方确认时间 (ms) */
#define UDS_TP_N_BS          1000u   /* 发送方流控等待 (ms) */
#define UDS_TP_N_CS          1000u   /* 发送方连续帧间隔 (ms) */
#define UDS_TP_ST_MIN        20u     /* 默认最小间隔 (ms) */
```

### 3.2 UDS 层时间参数 (ISO 14229-1)
```c
#define UDS_P2_SERVER_DEFAULT    50u      /* 默认响应超时 (ms) */
#define UDS_P2_SERVER_EXTENDED   5000u    /* 扩展响应超时 (ms) */
#define UDS_S3_SERVER_TIMEOUT    5000u    /* 会话超时 (ms) */
#define UDS_P2_STAR_MULTIPLIER   100u     /* P2* = P2 * 100 */
```

### 3.3 0x78 ResponsePending 参数
```c
#define UDS_NRC_78_INTERVAL      2000u    /* 0x78发送间隔 (ms) */
#define UDS_NRC_78_MAX_COUNT     3u       /* 最大连续发送次数 */
```

---

## 4. 架构设计

### 4.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                      │
│                   (用户自定义诊断服务实现)                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      UDS Service Layer                      │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │  Service    │ │  Session    │ │      Security        │  │
│  │  Dispatcher│ │   Manager   │ │       Manager        │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │  0x10 SVC   │ │  0x27 SVC   │ │      0x31 SVC        │  │
│  │ (Session)   │ │(Security)   │ │    (Routine)         │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      CAN TP Layer                           │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │ 单帧(SF)    │ │ 首帧(FF)    │ │    连续帧(CF)        │  │
│  │ 处理       │ │ 处理       │ │     处理             │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
│  ┌─────────────┐ ┌─────────────┐                           │
│  │ 流控帧(FC)  │ │ 多帧重组   │                           │  │
│  │ 处理       │ │ 缓冲区管理 │                           │  │
│  └─────────────┘ └─────────────┘                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    CAN Driver Layer                         │
│              (用户提供: CanFdSendQuick等)                   │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 模块职责

| 模块 | 职责 |
|------|------|
| uds_tp | CAN TP层协议实现（单帧/多帧、流控、重组） |
| uds_service | UDS服务层核心（服务分发、会话检查、安全验证、NRC处理、SPR处理） |
| uds_session | 会话管理（S3定时器、会话切换、超时处理） |
| uds_security | 安全访问管理（种子生成、密钥验证、尝试次数、算法函数指针） |
| uds_routine | 例程控制管理（RID配置表、权限检查） |
| uds_svc_XX | 具体诊断服务实现 |

---

## 5. 数据结构与类型定义

### 5.1 会话类型枚举
```c
typedef enum {
    UDS_SESSION_DEFAULT         = 0x01,   /* 默认会话 */
    UDS_SESSION_PROGRAMMING     = 0x02,   /* 编程会话 */
    UDS_SESSION_EXTENDED        = 0x03,   /* 扩展诊断会话 */
    UDS_SESSION_SECURITY        = 0x04    /* 安全会话 */
} UdsSessionType;
```

### 5.2 安全访问状态
```c
typedef enum {
    UDS_SECURITY_LOCKED     = 0x00,   /* 锁定状态 */
    UDS_SECURITY_UNLOCKED   = 0x01,   /* 解锁状态 */
    UDS_SECURITY_WAIT_KEY   = 0x02    /* 等待密钥 */
} UdsSecurityState;
```

### 5.3 否定响应码 (NRC)
```c
typedef enum {
    UDS_NRC_GENERAL_REJECT              = 0x10,
    UDS_NRC_SERVICE_NOT_SUPPORTED       = 0x11,
    UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED  = 0x12,
    UDS_NRC_INCORRECT_MESSAGE_LENGTH    = 0x13,
    UDS_NRC_CONDITIONS_NOT_CORRECT      = 0x22,
    UDS_NRC_REQUEST_SEQUENCE_ERROR      = 0x24,
    UDS_NRC_REQUEST_OUT_OF_RANGE        = 0x31,
    UDS_NRC_INVALID_KEY                 = 0x35,
    UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS   = 0x36,
    UDS_NRC_REQUIRED_TIME_DELAY         = 0x37,
    UDS_NRC_RESPONSE_PENDING            = 0x78,
    UDS_NRC_SUB_FUNC_NOT_SUPPORT_SESSION= 0x7E,
    UDS_NRC_SERVICE_NOT_SUPPORT_SESSION = 0x7F
} UdsNrcCode;
```

### 5.4 TP层帧类型
```c
typedef enum {
    UDS_TP_SF = 0,    /* 单帧 (Single Frame) */
    UDS_TP_FF,        /* 首帧 (First Frame) */
    UDS_TP_CF,        /* 连续帧 (Consecutive Frame) */
    UDS_TP_FC         /* 流控帧 (Flow Control) */
} UdsTpFrameType;
```

### 5.5 寻址类型
```c
typedef enum {
    UDS_ADDR_PHYSICAL   = 0,   /* 物理寻址 */
    UDS_ADDR_FUNCTIONAL = 1    /* 功能寻址 */
} UdsAddrType;
```

### 5.6 服务处理函数类型
```c
/* 服务处理函数指针类型 */
typedef UdsNrcCode (*UdsServiceHandler)(
    uint8_t *rxData, 
    uint16_t rxLen, 
    uint8_t *txData, 
    uint16_t *txLen
);
```

### 5.7 子服务配置结构体
```c
typedef struct {
    uint8_t             subServiceId;       /* 子服务ID */
    UdsServiceHandler   handler;            /* 处理函数 */
    uint32_t            sessionMask;        /* 允许会话的位图 */
    bool                needSecurity;       /* 是否需要安全访问 */
    uint8_t             securityLevel;      /* 所需安全等级 */
} UdsSubServiceConfig;
```

### 5.8 主服务配置结构体
```c
typedef struct {
    uint8_t                 serviceId;          /* 服务SID */
    UdsServiceHandler       defaultHandler;     /* 默认处理函数 */
    const UdsSubServiceConfig *subServiceTable; /* 子服务表 */
    uint8_t                 subServiceCount;    /* 子服务数量 */
} UdsServiceConfig;
```

### 5.9 安全访问算法函数指针类型
```c
/* 密钥计算函数类型 - 用户实现 */
typedef uint32_t (*UdsSecurityCalcKeyFunc)(uint32_t seed);

/* 安全访问等级配置 */
typedef struct {
    uint8_t                     level;          /* 安全等级 */
    UdsSecurityCalcKeyFunc      calcKeyFunc;    /* 密钥计算函数 */
    uint8_t                     maxAttempts;    /* 最大尝试次数 */
    uint32_t                    delayTime;      /* 锁定后延时(ms) */
} UdsSecurityLevelConfig;
```

### 5.10 例程控制 (0x31) RID配置结构体
```c
typedef enum {
    UDS_ROUTINE_START   = 0x01,   /* 启动例程 */
    UDS_ROUTINE_STOP    = 0x02,   /* 停止例程 */
    UDS_ROUTINE_REQUEST = 0x03    /* 请求结果 */
} UdsRoutineType;

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
```

### 5.11 TP层状态结构体
```c
typedef struct {
    /* 接收状态 */
    bool            rxInProgress;
    uint8_t         rxBuffer[UDS_TP_MAX_FRAME_SIZE];
    uint16_t        rxLength;
    uint16_t        rxIndex;
    uint8_t         rxExpectedSeq;
    uint16_t        rxBlockSize;
    uint32_t        rxTimer;
    
    /* 发送状态 */
    bool            txInProgress;
    uint8_t         txBuffer[UDS_TP_MAX_FRAME_SIZE];
    uint16_t        txLength;
    uint16_t        txIndex;
    uint8_t         txSeq;
    uint8_t         txBlockSize;
    uint8_t         txSTmin;
    uint32_t        txTimer;
    uint32_t        txFcWaitTimer;
    
    /* 流控状态 */
    bool            fcWait;
    uint8_t         fcStatus;       /* 0=继续, 1=等待, 2=溢出 */
} UdsTpContext;
```

### 5.12 0x78 ResponsePending 状态
```c
typedef struct {
    bool            active;         /* 是否正在发送0x78 */
    uint8_t         count;          /* 已发送次数 */
    uint32_t        timer;          /* 发送间隔定时器 */
    uint8_t         pendingService; /* 正在处理的服务SID */
} UdsPendingResponse;
```

### 5.13 UDS全局上下文
```c
typedef struct {
    /* 寻址信息 */
    UdsAddrType         addrType;
    bool                suppressPosResp;    /* 抑制肯定响应位 */
    
    /* 会话管理 */
    UdsSessionType      currentSession;
    uint32_t            s3Timer;
    bool                s3Active;
    
    /* 安全访问 */
    UdsSecurityState    securityState;
    uint8_t             securityLevel;
    uint8_t             securitySeed[4];
    uint8_t             failedAttempts;
    uint32_t            securityDelayTimer;
    const UdsSecurityLevelConfig *securityConfig;
    
    /* 时间参数 */
    uint32_t            p2Timer;
    bool                p2Active;
    bool                p2Extended;
    
    /* 0x78 ResponsePending */
    UdsPendingResponse  pendingResp;
    
    /* TP层上下文 */
    UdsTpContext        tpContext;
    
    /* 服务配置表 */
    const UdsServiceConfig *serviceTable;
    uint8_t             serviceCount;
    
    /* 例程配置表 */
    const UdsRoutineConfig *routineTable;
    uint8_t             routineCount;
} UdsContext;
```

---

## 6. 接口定义

### 6.1 初始化接口
```c
/* UDS协议栈初始化 */
void UdsInit(const UdsServiceConfig *serviceTable, uint8_t serviceCount);

/* 安全访问配置初始化 */
void UdsSecurityConfigInit(const UdsSecurityLevelConfig *config);

/* 例程控制配置初始化 */
void UdsRoutineConfigInit(const UdsRoutineConfig *routineTable, uint8_t routineCount);

/* CAN TP层初始化 */
void UdsTpInit(void);

/* 会话管理初始化 */
void UdsSessionInit(void);

/* 安全访问初始化 */
void UdsSecurityInit(void);
```

### 6.2 底层事件接口（用户调用）
```c
/* CAN接收指示 - 在中断中调用 */
void UdsCanRxIndication(uint32_t canId, uint8_t *data, uint8_t length);

/* 1ms定时器中断 - 在LPIT_ISR中调用 */
void UdsTimerISR(void);
```

### 6.3 内部核心接口
```c
/* UDS主处理 - 在接收完成后调用 */
void UdsProcessMessage(uint8_t *data, uint16_t length);

/* 发送肯定响应 */
void UdsSendPositiveResponse(uint8_t serviceId, uint8_t *data, uint16_t length);

/* 发送否定响应 */
void UdsSendNegativeResponse(uint8_t serviceId, UdsNrcCode nrc);

/* 发送0x78 ResponsePending */
void UdsSendResponsePending(uint8_t serviceId);

/* 检查会话权限 */
bool UdsCheckSessionPermission(uint32_t sessionMask);

/* 检查安全访问权限 */
bool UdsCheckSecurityPermission(uint8_t requiredLevel);

/* 检查抑制肯定响应位 */
bool UdsCheckSuppressPosResp(uint8_t subService);
```

### 6.4 TP层接口
```c
/* TP层接收处理 */
void UdsTpRxIndication(uint8_t *data, uint8_t length);

/* TP层发送请求 */
status_t UdsTpTransmit(uint8_t *data, uint16_t length);

/* TP层定时器处理 */
void UdsTpTimerHandler(void);
```

### 6.5 服务层接口（供具体服务实现调用）
```c
/* 获取当前会话 */
UdsSessionType UdsGetCurrentSession(void);

/* 切换会话 */
void UdsSetSession(UdsSessionType session);

/* 获取安全状态 */
UdsSecurityState UdsGetSecurityState(void);

/* 解锁安全访问 */
void UdsUnlockSecurity(uint8_t level);

/* 锁定安全访问 */
void UdsLockSecurity(void);

/* 启动0x78定时器 */
void UdsStartPendingResponse(uint8_t serviceId);

/* 停止0x78定时器 */
void UdsStopPendingResponse(void);
```

### 6.6 例程控制接口
```c
/* 查找RID配置 */
const UdsRoutineConfig* UdsRoutineFind(uint16_t rid);

/* 检查RID权限 */
bool UdsRoutineCheckPermission(const UdsRoutineConfig *config);
```

---

## 7. 服务配置表示例

### 7.1 0x10 诊断会话控制 - 子服务表
```c
const UdsSubServiceConfig gUdsSvc10SubTable[] = {
    /* 子服务ID, 处理函数, 会话权限, 需安全访问, 安全等级 */
    {0x01, UdsSvc10DefaultSession,     (1<<UDS_SESSION_DEFAULT),                   false, 0},
    {0x02, UdsSvc10ProgrammingSession, (1<<UDS_SESSION_DEFAULT)|(1<<UDS_SESSION_EXTENDED), false, 0},
    {0x03, UdsSvc10ExtendedSession,    (1<<UDS_SESSION_DEFAULT)|(1<<UDS_SESSION_EXTENDED), false, 0},
    {0x04, UdsSvc10SecuritySession,    (1<<UDS_SESSION_EXTENDED),                  true,  1},
};
```

### 7.2 0x27 安全访问 - 子服务表
```c
const UdsSubServiceConfig gUdsSvc27SubTable[] = {
    /* 子服务ID, 处理函数, 会话权限, 需安全访问, 安全等级 */
    {0x01, UdsSvc27RequestSeed,  (1<<UDS_SESSION_EXTENDED)|(1<<UDS_SESSION_SECURITY), false, 0},
    {0x02, UdsSvc27SendKey,      (1<<UDS_SESSION_EXTENDED)|(1<<UDS_SESSION_SECURITY), false, 0},
};
```

### 7.3 0x27 安全访问算法配置
```c
/* 用户实现的密钥计算函数 */
uint32_t MyCalcKeyFunc(uint32_t seed)
{
    /* 用户自定义算法 */
    return seed ^ 0x12345678;
}

const UdsSecurityLevelConfig gSecurityLevelConfig[] = {
    {1, MyCalcKeyFunc, 3, 10000},  /* Level 1, 最大3次尝试, 锁定10秒 */
};
```

### 7.4 0x31 例程控制 - RID配置表
```c
/* 例程1: 0x0201 - 擦除内存 */
UdsNrcCode RoutineEraseStart(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);
UdsNrcCode RoutineEraseStop(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);
UdsNrcCode RoutineEraseResult(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);

/* 例程2: 0xFF00 - 自检 */
UdsNrcCode RoutineSelfTestStart(uint8_t *data, uint16_t len, uint8_t *resp, uint16_t *respLen);

const UdsRoutineConfig gUdsRoutineTable[] = {
    /* RID, 会话权限, 需安全访问, 安全等级, Start回调, Stop回调, RequestResult回调, IsRunning */
    {0x0201, (1<<UDS_SESSION_PROGRAMMING), true, 1, 
     RoutineEraseStart, RoutineEraseStop, RoutineEraseResult, NULL},
    {0xFF00, (1<<UDS_SESSION_EXTENDED), false, 0,
     RoutineSelfTestStart, NULL, NULL, NULL},
};
```

### 7.5 主服务配置表
```c
const UdsServiceConfig gUdsServiceTable[] = {
    {0x10, NULL, gUdsSvc10SubTable, sizeof(gUdsSvc10SubTable)/sizeof(gUdsSvc10SubTable[0])},
    {0x27, NULL, gUdsSvc27SubTable, sizeof(gUdsSvc27SubTable)/sizeof(gUdsSvc27SubTable[0])},
    {0x31, UdsSvc31Handler, NULL, 0},  /* 0x31使用自定义处理 */
};
```

---

## 8. 状态机设计

### 8.1 会话状态机
```
                    ┌───────────────┐
                    │  默认会话(01) │◄────────────────┐
                    │  S3定时器关闭 │                 │
                    └───────┬───────┘                 │
                            │ 0x1002/0x1003           │
                            ▼                         │
             ┌──────────────────────────┐            │
             │      扩展会话(03)        │            │
             │   S3定时器启动(5秒)      │            │
             │   超时自动返回默认会话   │────────────┘
             └──────────┬───────────────┘ 0x1001
                        │ 0x1004
                        ▼
             ┌──────────────────────────┐
             │      安全会话(04)        │
             │   需要安全访问(0x27)     │
             │   S3定时器持续运行       │
             └──────────────────────────┘
```

### 8.2 安全访问状态机
```
                    ┌───────────────┐
                    │   锁定状态    │
                    │ (Level 0)     │
                    └───────┬───────┘
                            │ 0x2701 (请求种子)
                            ▼
                    ┌───────────────┐
                    │ 等待密钥状态  │
                    │ (4字节种子)   │◄─────┐
                    └───────┬───────┘      │
                            │ 0x2702       │
              ┌─────────────┼─────────────┐│
              │             │             ││
              ▼             ▼             ▼│
        ┌─────────┐   ┌─────────┐   ┌──────────┐
        │密钥正确│   │密钥错误 │   │尝试超限  │
        │解锁成功│   │计数+1   │   │锁定延时  │
        └────┬────┘   └────┬────┘   └────┬─────┘
             │             │             │
             ▼             └──────►──────┘
        ┌─────────┐
        │解锁状态 │
        │Level 1  │
        └─────────┘
```

### 8.3 0x78 ResponsePending 机制
```
服务处理开始
    │
    ▼
┌─────────────────┐
│ 处理完成?       │────是────► 发送肯定响应
└────────┬────────┘
         │否
         ▼
┌─────────────────┐
│ P2超时接近?     │────否────► 继续处理
└────────┬────────┘
         │是
         ▼
┌─────────────────┐     ┌─────────────┐
│ 发送0x78        │────►│ 重置P2定时器│
│ 计数+1          │     │ 继续处理    │
└────────┬────────┘     └─────────────┘
         │
    ┌────┴────┐
    ▼         ▼
 计数超限   处理完成
 发送NRC    发送响应
 0x78      (肯定或否定)
```

---

## 9. 多帧传输流程

### 9.1 接收多帧流程
```
CAN接收中断
    │
    ▼
┌─────────────────┐
│ 首帧(FF)接收?   │────否────► 单帧处理 ──► UDS处理
└────────┬────────┘
         │是
         ▼
┌─────────────────┐
│ 发送流控帧(FC)  │  BlockSize=8, STmin=20ms
│ 启动N_Cr定时器  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 接收连续帧(CF)  │◄──────────────────┐
│ 重组数据        │                   │
│ 检查序列号      │                   │
└────────┬────────┘                  │
         │                          │
         ▼ 未完成                   │ 完成
    ┌─────────┐                     │
    │ BS计数  │────计数满────►发送FC─┘
    └─────────┘
```

### 9.2 发送多帧流程
```
UDS层请求发送多帧
    │
    ▼
┌─────────────────┐
│ 发送首帧(FF)    │
│ 启动N_Bs定时器  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 等待流控帧(FC)  │◄──────────────────┐
└────────┬────────┘                  │
         │                          │
    ┌────┴────┐                     │
    ▼         ▼                     │
  超时       收到FC                  │
  报错       按BS和STmin             │
             发送连续帧              │
             ────────────────────────┘
```

---

## 10. 功能寻址与SPR处理

### 10.1 寻址判断
```c
void UdsCanRxIndication(uint32_t canId, uint8_t *data, uint8_t length)
{
    if (canId == UDS_FUNC_REQ_ADDR) {
        gUdsContext.addrType = UDS_ADDR_FUNCTIONAL;
    } else if (canId == UDS_PHYS_REQ_ADDR) {
        gUdsContext.addrType = UDS_ADDR_PHYSICAL;
    } else {
        return;  /* 不处理的CAN ID */
    }
    /* ... */
}
```

### 10.2 SPR位检查
```c
bool UdsCheckSuppressPosResp(uint8_t subService)
{
    /* bit7为1表示抑制肯定响应 */
    return ((subService & 0x80u) != 0u);
}
```

### 10.3 SPR处理规则
```c
/* 服务分发时处理 */
void UdsProcessMessage(uint8_t *data, uint16_t length)
{
    uint8_t serviceId = data[0];
    uint8_t subService = data[1];
    
    /* 检查SPR位 */
    if (UdsCheckSuppressPosResp(subService)) {
        gUdsContext.suppressPosResp = true;
        /* 清除SPR位后进行后续处理 */
        subService &= 0x7Fu;
    } else {
        gUdsContext.suppressPosResp = false;
    }
    
    /* 功能寻址时，不支持的服务不发送否定响应 */
    if (gUdsContext.addrType == UDS_ADDR_FUNCTIONAL) {
        if (!IsServiceSupported(serviceId)) {
            return;  /* 静默丢弃 */
        }
    }
    
    /* ... 服务处理 ... */
}
```

### 10.4 响应发送规则
| 寻址方式 | SPR=0 | SPR=1 |
|---------|-------|-------|
| 物理寻址 | 发送肯定/否定响应 | 只发送否定响应 |
| 功能寻址 | 发送肯定/否定响应 | 不发送任何响应 |

---

## 11. 文件结构

```
Sources/uds/
├── uds_config.h              # 用户配置（地址、时间参数、缓冲区大小）
├── uds_types.h               # 类型定义、枚举、结构体
├── uds_platform.h            # 平台抽象层接口声明
├── uds_core.h/.c             # UDS核心（初始化、消息分发、SPR处理）
├── uds_tp.h/.c               # CAN TP层实现
├── uds_session.h/.c          # 会话管理
├── uds_security.h/.c         # 安全访问管理（含算法函数指针）
├── uds_routine.h/.c          # 例程控制管理（RID配置表）
├── uds_pending.h/.c          # 0x78 ResponsePending处理
├── uds_nrc.h/.c              # 否定响应处理
└── services/
    ├── uds_svc_10.h/.c       # 诊断会话控制（完整实现）
    ├── uds_svc_27.h/.c       # 安全访问（框架+0x2701，函数指针）
    └── uds_svc_31.h/.c       # 例程控制（RID配置表驱动）
```

---

## 12. 关键设计决策

### 12.1 配置表驱动
- 优点：服务扩展只需修改配置表，无需改动核心代码
- 子服务权限通过位图和布尔值灵活配置
- 0x31服务采用RID配置表，支持灵活的例程定义

### 12.2 安全访问算法函数指针
```c
/* 用户通过配置表传入密钥计算函数 */
const UdsSecurityLevelConfig gSecurityLevelConfig[] = {
    {1, MyCalcKeyFunc, 3, 10000},
};
```

### 12.3 0x78 ResponsePending 机制
- 支持连续发送0x78
- 可配置发送间隔和最大次数
- 超过最大次数后发送NRC 0x78作为最终否定响应

### 12.4 单请求处理
- 只维护一个请求处理状态机，简化设计
- 新请求到来时中止当前处理（返回适当NRC）

### 12.5 缓冲区设计
- Rx/Tx缓冲区大小: 4095字节（ISO 15765-2最大值）
- 静态分配，避免动态内存分配

### 12.6 定时器设计
- 所有定时器基于1ms中断递减计数
- 超时检查在主循环或中断中处理

### 12.7 功能寻址与SPR
- 自动检测寻址类型
- 支持抑制肯定响应位处理
- 功能寻址时不支持的服务静默丢弃

---

## 13. 使用示例

### 13.1 初始化流程
```c
/* main.c 或初始化函数中 */
void UdsStackInit(void)
{
    /* 初始化TP层 */
    UdsTpInit();
    
    /* 初始化会话管理 */
    UdsSessionInit();
    
    /* 初始化安全访问 */
    UdsSecurityInit();
    UdsSecurityConfigInit(gSecurityLevelConfig);
    
    /* 初始化例程控制 */
    UdsRoutineConfigInit(gUdsRoutineTable, 
        sizeof(gUdsRoutineTable)/sizeof(gUdsRoutineTable[0]));
    
    /* 初始化UDS核心 */
    UdsInit(gUdsServiceTable, 
        sizeof(gUdsServiceTable)/sizeof(gUdsServiceTable[0]));
}
```

### 13.2 CAN接收中断
```c
void CAN0_Callback_Func(uint32_t instance, can_event_t event, 
                        uint32_t buffIdx, void *flexcanState)
{
    (void)flexcanState;
    (void)instance;
    
    if (event == CAN_EVENT_RX_COMPLETE) {
        can_message_t rxMsg;
        CAN_Receive(&can_pal0_instance, RX_MAILBOX_CAN0, &rxMsg);
        
        /* 调用UDS接收指示 */
        UdsCanRxIndication(rxMsg.id, rxMsg.data, rxMsg.length);
    }
}
```

### 13.3 定时器中断
```c
void LPIT_ISR(void)
{
    /* 调用UDS定时器处理 */
    UdsTimerISR();
    
    /* 其他定时任务... */
}
```

---

**文档版本**: v2.0  
**日期**: 2026-03-08  
**状态**: 待Review
