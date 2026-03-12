#ifndef __UDS_APP_H
#define __UDS_APP_H

#include "stm32f4xx.h"  

/* ======================
 * 会话掩码定义 (Session Mask)
 * 采用按位掩码设计，方便一个服务在多个会话下都生效
 * ====================== */
#define SESSION_MASK_DEFAULT     (1 << 0)  /* 默认会话 0x01 */
#define SESSION_MASK_PROGRAMMING (1 << 1)  /* 编程会话 0x02 */
#define SESSION_MASK_EXTENDED    (1 << 2)  /* 扩展会话 0x03 */

/* ======================
 * 安全等级掩码定义 (Security Mask)
 * 采用按位掩码，支持 0x27 的不同等级解锁
 * ====================== */
#define SECURITY_MASK_LOCKED     0x00      /* 完全未解锁, 0x00 表示无需解锁直接访问 */
#define SECURITY_MASK_LEVEL_1    (1 << 0)  /* 满足 27 01/02 解锁 */
#define SECURITY_MASK_LEVEL_3    (1 << 1)  /* 满足 27 03/04 解锁 (供应商专属等) */
/* 如有需要，可任意往后加 (1<<2), (1<<3)... */

/* ======================
 * UDS 常见服务 ID (SID)
 * ====================== */
#define UDS_SID_DIAGNOSTIC_SESSION_CONTROL  0x10
#define UDS_SID_ECU_RESET                   0x11
#define UDS_SID_READ_DATA_BY_IDENTIFIER     0x22
#define UDS_SID_SECURITY_ACCESS             0x27
#define UDS_SID_ROUTINE_CONTROL             0x31

#define UDS_SID_REQUEST_DOWNLOAD            0x34
#define UDS_SID_TRANSFER_DATA               0x36
#define UDS_SID_REQUEST_TRANSFER_EXIT       0x37
#define UDS_SID_TESTER_PRESENT              0x3E

/* NRC (Negative Response Code - 否定响应码) */
#define NRC_GENERAL_REJECT                  0x10
#define NRC_SERVICE_NOT_SUPPORTED           0x11
#define NRC_SUB_FUNCTION_NOT_SUPPORTED      0x12
#define NRC_INCORRECT_MESSAGE_LENGTH        0x13
#define NRC_CONDITIONS_NOT_CORRECT          0x22
#define NRC_REQUEST_SEQUENCE_ERROR          0x24
#define NRC_SECURITY_ACCESS_DENIED          0x33

/* ======================
 * 状态机定义
 * ====================== */
typedef enum {
    UDS_SESSION_DEFAULT = 1,
    UDS_SESSION_PROGRAMMING = 2,
    UDS_SESSION_EXTENDED = 3
} UdsSession;

typedef enum {
    UDS_FLASH_STATE_IDLE = 0,
    UDS_FLASH_STATE_REQUESTED, 
    UDS_FLASH_STATE_TRANSFERING
} UdsFlashState;

typedef struct {
    UdsSession session;        /* 当前会话状态 (枚举值) */
    uint8_t currentSessionMask;/* 方便快速按位与(AND)比对当前的掩码 */
    uint8_t securityMask;      /* 当前解锁的所有安全等级集合 */
    
    /* 固件刷写上下文 */
    UdsFlashState flashState; 
    uint32_t expectedMemoryAddress;
    uint32_t totalMemorySize;       
    uint16_t blockSequenceCounter;
    uint32_t transferredSize;       
} UdsContext;

extern UdsContext udsCtx;


/* ======================
 * 服务回调函数与表节点类型定义
 * ====================== */
typedef void (*UdsCallback)(uint8_t *data, uint16_t len);

/* --- 子服务(Sub-Function) 配置表节点 --- */
typedef struct {
    uint8_t subFunc;
    uint8_t sessionMask;   /* 允许的会话掩码 */
    uint8_t securityMask;  /* 允许的安全等级掩码 (0x00代表无需解锁) */
    UdsCallback callback;  /* 具体子服务业务逻辑 */
} UdsSubServiceConfig;

/* --- 主服务(Service) 配置表节点 --- */
typedef struct {
    uint8_t sid;
    uint8_t sessionMask;   /* 主服务允许的会话掩码 */
    uint8_t securityMask;  /* 主服务允许的安全等级掩码 */
    uint8_t hasSubFunc;    /* 标志位: 1表示具备子服务(如0x10, 0x11), 0表示直接执行 */
    
    UdsCallback callback;  /* 不带子功能时的直接业务回调 */
    
    const UdsSubServiceConfig *subServiceTable; /* 子功能表指针集 (可配置NULL) */
    uint8_t subServiceCount;                    /* 子功能表项数量 */
} UdsServiceConfig;


/* ======================
 * 对外开放的接口
 * ====================== */

/**
  * @brief      初始化UDS应用层上下文
  * @details    复位所有会话状态、安全等级掩码和刷写进度参数。
  * @return     无
  * @note       需要在系统复位后调用一次。
  * @warning    未调用此函数将导致UDS服务无法正常响应。
  * @author     小昭debug
  * @date       2026-02-26
  */
void UdsInit(void);

/**
  * @brief      发送 UDS 否定响应 (NRC)
  * @details    针对不支持的服务或异常状态，发送 0x7F + SID + NRC 的否定响应。
  * @param[in]  sid 产生报错的服务ID (如 0x34)
  * @param[in]  nrc 否定响应码 (如 0x13 长度错误)
  * @return     无
  * @note       底层通过 TP 的单帧(SF)和物理响应ID发送。
  * @author     小昭debug
  * @date       2026-02-26
  */
void UdsSendNegativeResponse(uint8_t sid, uint8_t nrc);

/**
  * @brief      UDS 消息的主入口处理函数 (表驱动路由)
  * @details    由网络层(TP层)接收组包成功后进行回调调用，
  *             内部自动完成会话校验和安全等级校验，
  *             若服务包含子功能则进入二级子表查找并执行对应回调。
  * @param[in]  data 指向完整的 UDS Payload 数据指针
  * @param[in]  len  数据总长度
  * @return     无
  * @note       所有权限校验均在此函数内完成，业务回调无需重复判断。
  * @author     小昭debug
  * @date       2026-02-26
  */
void UdsProcessMsg(uint8_t *data, uint16_t len);

#endif
