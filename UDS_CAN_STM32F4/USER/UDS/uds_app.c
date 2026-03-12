#include "uds_app.h"
#include "iso15765_tp.h" 
#include <stdio.h>       

UdsContext udsCtx;

/* 前置声明业务回调函数 */
static void UdsHandle10_01(uint8_t *data, uint16_t len);
static void UdsHandle10_02(uint8_t *data, uint16_t len);
static void UdsHandle10_03(uint8_t *data, uint16_t len);

static void UdsHandleService22(uint8_t *data, uint16_t len);
static void UdsHandleService27(uint8_t *data, uint16_t len);
static void UdsHandleService34(uint8_t *data, uint16_t len);
static void UdsHandleService36(uint8_t *data, uint16_t len);
static void UdsHandleService37(uint8_t *data, uint16_t len);
static void UdsHandleService3E(uint8_t *data, uint16_t len);

/* =================================================================================
 * [二维查表配置区]
 * 所有的服务权限、子功能权限，全部在这里以纯数据表的形式进行解耦配置！
 *
 * 字段含义: 
 *   {SubFunc, 会话掩码(可多选), 安全等级掩码, 业务回调函数}
 * ================================================================================= */

/* ---- 0x10 诊断会话控制 子功能表 ---- */
static const UdsSubServiceConfig gSubTable_10[] = {
    /* 01: 默认会话, 不需要安全访问 */
    {0x01, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, UdsHandle10_01},
    /* 02: 编程会话, 不需要安全访问 */
    {0x02, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, UdsHandle10_02},
    /* 03: 扩展会话, 不需要安全访问 */
    {0x03, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, UdsHandle10_03},
};

/* TODO: 如有需要，在这里添加 0x11, 0x31 的子功能表 */


/* =================================================================================
 * [主服务路由表总入口]
 * 字段含义:
 *   {SID, 会话掩码, 安全掩码, 是否有子功能(1/0), 无子功能的直接回调, 子功能表指针, 子表长度}
 * ================================================================================= */
static const UdsServiceConfig gUdsServiceTable[] = {
    /* 0x10 服务支持子功能路由, 不需要安全访问 */
    {0x10, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, 1, 0, gSubTable_10, sizeof(gSubTable_10)/sizeof(UdsSubServiceConfig)},
    
    /* 0x22 读DID服务, 不需要解锁 */
    {0x22, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, 0, UdsHandleService22, 0, 0},
    
    /* 0x27 安全访问服务, 不需要解锁(本身就是做解锁的) */
    {0x27, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, 0, UdsHandleService27, 0, 0},

    /* 0x34 请求下载: 必须在[PROGRAMMING]会话 并且 [已解锁 LEVEL_1 或 LEVEL_3] */
    {0x34, SESSION_MASK_PROGRAMMING, SECURITY_MASK_LEVEL_1 | SECURITY_MASK_LEVEL_3, 0, UdsHandleService34, 0, 0},
    
    /* 0x36 传输数据: 同 0x34 权限 */
    {0x36, SESSION_MASK_PROGRAMMING, SECURITY_MASK_LEVEL_1 | SECURITY_MASK_LEVEL_3, 0, UdsHandleService36, 0, 0},
    
    /* 0x37 退出传输: 同 0x34 权限 */
    {0x37, SESSION_MASK_PROGRAMMING, SECURITY_MASK_LEVEL_1 | SECURITY_MASK_LEVEL_3, 0, UdsHandleService37, 0, 0},
    
    /* 0x3E 测试仪在线: 任何会话下都可以, 不需要解锁 */
    {0x3E, SESSION_MASK_DEFAULT|SESSION_MASK_PROGRAMMING|SESSION_MASK_EXTENDED, SECURITY_MASK_LOCKED, 0, UdsHandleService3E, 0, 0}
};

/* 表格计算出条数 */
#define UDS_SERVICE_TABLE_SIZE (sizeof(gUdsServiceTable) / sizeof(UdsServiceConfig))


/* =================================================================================
 * [基础公共函数]
 * ================================================================================= */

/**
  * @brief      初始化UDS应用层上下文
  * @details    复位所有会话状态、安全等级掩码和刷写进度参数。
  * @return     无
  * @note       需要在系统复位后调用一次。
  * @warning    未调用此函数将导致UDS服务无法正常响应。
  * @author     小昭debug
  * @date       2026-02-26
  */
void UdsInit(void) {
    udsCtx.session = UDS_SESSION_DEFAULT;
    udsCtx.currentSessionMask = SESSION_MASK_DEFAULT;
    udsCtx.securityMask = SECURITY_MASK_LOCKED;  /* 开机一定是未解锁 */
    
    udsCtx.flashState = UDS_FLASH_STATE_IDLE;
    udsCtx.expectedMemoryAddress = 0;
    udsCtx.totalMemorySize = 0;
    udsCtx.transferredSize = 0;
    udsCtx.blockSequenceCounter = 0;
    printf("[UDS Table-Driven] Init completed.\r\n");
}

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
void UdsSendNegativeResponse(uint8_t sid, uint8_t nrc) {
    uint8_t txBuf[3];
    txBuf[0] = 0x7F; 
    txBuf[1] = sid;
    txBuf[2] = nrc;

    printf("[UDS_NRC] Reject SID:%02X with NRC:0x%02X\r\n", sid, nrc);
    TpSendSingleFrame(txBuf, 3);
}

/**
  * @brief      校验当前会话是否满足服务要求
  * @details    将配置表中的会话掩码与当前上下文的会话掩码进行按位与运算。
  * @param[in]  allowedMask 配置表中允许的会话掩码
  * @return     uint8_t 1:校验通过, 0:不满足
  * @author     小昭debug
  * @date       2026-02-26
  */
static uint8_t UdsCheckSession(uint8_t allowedMask) {
    if ((allowedMask & udsCtx.currentSessionMask) != 0) {
        return 1;
    }
    return 0;
}

/**
  * @brief      校验当前安全等级是否满足服务要求
  * @details    对比配置表中的安全掩码与当前上下文已解锁的安全等级掩码。
  *             当 allowedMask 为 0 (SECURITY_MASK_LOCKED) 时表示该服务无需解锁。
  *             当 allowedMask 非 0 时，需要当前已解锁的等级中存在与之匹配的位。
  * @param[in]  allowedMask 配置表中允许的安全等级掩码
  * @return     uint8_t 1:校验通过, 0:安全等级不足
  * @author     小昭debug
  * @date       2026-02-26
  */
static uint8_t UdsCheckSecurity(uint8_t allowedMask) {
    /* 配置为 0 (SECURITY_MASK_LOCKED) 表示该服务无需任何安全解锁 */
    if (allowedMask == 0) {
        return 1;
    }
    
    /* 配置了非零掩码，需要当前解锁等级中存在匹配位 */
    if ((allowedMask & udsCtx.securityMask) != 0) {
        return 1;
    }
    
    /* 未匹配到所需的安全等级 */
    return 0;
}


/* =================================================================================
 * [表驱动核心解析路由]
 * 所有的 UDS 请求，全都会经过这个二维查表器
 * ================================================================================= */

/**
  * @brief      UDS 消息的主入口处理函数 (表驱动路由)
  * @details    由网络层(TP层)接收组包成功后进行回调调用。
  *             内部通过遍历 gUdsServiceTable 主表查找对应的 SID，
  *             自动完成会话校验和安全等级校验，
  *             若服务包含子功能则进入二级子表查找并执行对应回调。
  * @param[in]  data 指向完整的 UDS Payload 数据指针
  * @param[in]  len  数据总长度
  * @return     无
  * @note       所有权限校验均在此函数内完成，业务回调无需重复判断。
  * @author     小昭debug
  * @date       2026-02-26
  */
void UdsProcessMsg(uint8_t *data, uint16_t len) {
    uint8_t sid;
    uint8_t subFunc;
    uint8_t i, j;
    uint8_t serviceFound = 0;
    uint8_t subFuncFound = 0;
    
    if(len == 0 || data == 0) return;
    
    sid = data[0]; 
    printf("\r\n[UDS] Rx SID: %02X, Len: %d\r\n", sid, len);
    
    /* 1. 一级查表：查找主服务 (Service) */
    for(i = 0; i < UDS_SERVICE_TABLE_SIZE; i++) {
        const UdsServiceConfig *pSrv = &gUdsServiceTable[i];
        
        if (pSrv->sid == sid) {
            serviceFound = 1;
            
            /* 权限校验1：校验主服务的会话状态 */
            if (!UdsCheckSession(pSrv->sessionMask)) {
                UdsSendNegativeResponse(sid, NRC_CONDITIONS_NOT_CORRECT);
                return;
            }
            
            /* 权限校验2：校验主服务的安全访问等级 */
            if (!UdsCheckSecurity(pSrv->securityMask)) {
                UdsSendNegativeResponse(sid, NRC_SECURITY_ACCESS_DENIED);
                return;
            }
            
            /* 如果包含子功能(Sub-Function)，则进入二级查表 */
            if (pSrv->hasSubFunc) {
                if (len < 2) {
                    UdsSendNegativeResponse(sid, NRC_INCORRECT_MESSAGE_LENGTH);
                    return;
                }
                
                subFunc = data[1] & 0x7F; /* 忽略正响应抑制位 */
                
                for(j = 0; j < pSrv->subServiceCount; j++) {
                    const UdsSubServiceConfig *pSubSrv = &pSrv->subServiceTable[j];
                    
                    if (pSubSrv->subFunc == subFunc) {
                        subFuncFound = 1;
                        
                        /* 二层权限校验：子功能独立的会话权限 */
                        if (!UdsCheckSession(pSubSrv->sessionMask)) {
                            UdsSendNegativeResponse(sid, NRC_SUB_FUNCTION_NOT_SUPPORTED);
                            return;
                        }
                        
                        /* 二层权限校验：子功能独立的安全等级 */
                        if (!UdsCheckSecurity(pSubSrv->securityMask)) {
                            UdsSendNegativeResponse(sid, NRC_SECURITY_ACCESS_DENIED);
                            return;
                        }
                        
                        /* 全部校验通过，执行具体回调 */
                        printf("[UDS] Executing Sub-funct %02X callback\r\n", subFunc);
                        if(pSubSrv->callback) {
                            pSubSrv->callback(data, len);
                        }
                        return;
                    }
                }
                
                /* 主服务找到了，但子服务在表里没配 */
                if (!subFuncFound) {
                    UdsSendNegativeResponse(sid, NRC_SUB_FUNCTION_NOT_SUPPORTED);
                    return;
                }
                
            } else {
                /* 该主服务没有子功能，直接触发主业务回调 */
                printf("[UDS] Executing Main Service %02X callback\r\n", sid);
                if(pSrv->callback) {
                    pSrv->callback(data, len);
                }
                return;
            }
        }
    }
    
    /* 主表里找不到该 SID */
    if (!serviceFound) {
        printf("[UDS] Unsupported SID: %02X\r\n", sid);
        UdsSendNegativeResponse(sid, NRC_SERVICE_NOT_SUPPORTED);
    }
}


/* =================================================================================
 * [业务功能具体实现区]
 * 进入以下函数时，会话和安全等级校验已全部通过，只需关注业务逻辑。
 * ================================================================================= */

/**
  * @brief      0x10 会话切换公共回复函数
  * @details    统一构造 0x50 正响应 + P2/P2* 时序参数并发送。
  * @param[in]  subFunc 子功能编号 (0x01/0x02/0x03)
  * @return     无
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandle10_CommonReply(uint8_t subFunc) {
    uint8_t txBuf[6];
    
    txBuf[0] = 0x50;
    txBuf[1] = subFunc;
    txBuf[2] = 0x00; /* P2 max high */
    txBuf[3] = 0x32; /* P2 max low (50ms) */
    txBuf[4] = 0x01; /* P2* max high */
    txBuf[5] = 0xF4; /* P2* max low (500ms) */
    
    TpSendSingleFrame(txBuf, 6);
    printf("[UDS_10] Session Change to 0x%02X Done.\r\n", subFunc);
}

/**
  * @brief      处理 0x10 01 切换到默认会话
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandle10_01(uint8_t *data, uint16_t len) {
    udsCtx.session = UDS_SESSION_DEFAULT;
    udsCtx.currentSessionMask = SESSION_MASK_DEFAULT;
    UdsHandle10_CommonReply(0x01);
}

/**
  * @brief      处理 0x10 02 切换到编程会话
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandle10_02(uint8_t *data, uint16_t len) {
    udsCtx.session = UDS_SESSION_PROGRAMMING;
    udsCtx.currentSessionMask = SESSION_MASK_PROGRAMMING;
    UdsHandle10_CommonReply(0x02);
}

/**
  * @brief      处理 0x10 03 切换到扩展会话
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandle10_03(uint8_t *data, uint16_t len) {
    udsCtx.session = UDS_SESSION_EXTENDED;
    udsCtx.currentSessionMask = SESSION_MASK_EXTENDED;
    UdsHandle10_CommonReply(0x03);
}

/**
  * @brief      处理 0x22 读DID服务
  * @details    根据DID读取对应的数据(如软件版本号、电压等)。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @note       TODO: 需根据实际业务扩展DID读取逻辑。
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService22(uint8_t *data, uint16_t len) {
    printf("[UDS_22] Read DID...\r\n");
    UdsSendNegativeResponse(UDS_SID_READ_DATA_BY_IDENTIFIER, NRC_REQUEST_SEQUENCE_ERROR);
}

/**
  * @brief      处理 0x27 安全访问服务 (Seed/Key)
  * @details    根据子功能号进行种子请求(01/03)或密钥校验(02/04)。
  *             校验成功后通过 udsCtx.securityMask 打上对应的解锁标志位。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @note       TODO: 需实现具体的 Seed 生成和 Key 校验算法。
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService27(uint8_t *data, uint16_t len) {
    uint8_t subFunc;
    
    if(len < 2) {
        UdsSendNegativeResponse(UDS_SID_SECURITY_ACCESS, NRC_INCORRECT_MESSAGE_LENGTH);
        return;
    }
    subFunc = data[1] & 0x7F;
    
    printf("[UDS_27] Security Access Level %02X...\r\n", subFunc);
    
    if(subFunc == 0x01 || subFunc == 0x03) {
        /* TODO: 生成并返回 Seed */
    } 
    else if (subFunc == 0x02 || subFunc == 0x04) {
        /* TODO: 校验客户端送回来的 Key，成功后打上对应的标志位 
           例如: udsCtx.securityMask |= SECURITY_MASK_LEVEL_1; */
    }
    
    UdsSendNegativeResponse(UDS_SID_SECURITY_ACCESS, NRC_SERVICE_NOT_SUPPORTED);
}

/**
  * @brief      处理 0x3E 测试仪在线心跳
  * @details    收到后根据抑制位决定是否回复正响应。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService3E(uint8_t *data, uint16_t len) {
    uint8_t txHeartbeat = 0x7E;
    printf("[UDS_3E] Tester Present.\r\n");
    if(len >= 2 && data[1] != 0x80) { 
        TpSendSingleFrame(&txHeartbeat, 1);
    }
}

/**
  * @brief      处理 0x34 请求下载 (Request Download)
  * @details    解析测试仪发来的内存地址和长度，初始化刷写上下文，回复肯定响应。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @note       进入此函数时，已确保处于编程会话且安全解锁。
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService34(uint8_t *data, uint16_t len) {
    uint8_t format;
    uint8_t memSizeLen;
    uint8_t memAddrLen;
    uint8_t txBuf[4];
       
    printf("[UDS_34] RX 0x34 Request Download\r\n");
    
    if(len < 5) {
        UdsSendNegativeResponse(UDS_SID_REQUEST_DOWNLOAD, NRC_INCORRECT_MESSAGE_LENGTH);
        return;
    }
    
    format = data[1];             
    memSizeLen = (format >> 4) & 0x0F;
    memAddrLen = format & 0x0F;
    
    if(memAddrLen == 4) {
        udsCtx.expectedMemoryAddress = ((uint32_t)data[2]<<24) | ((uint32_t)data[3]<<16) | ((uint32_t)data[4]<<8) | data[5];
    }
    if(memSizeLen == 4) {
        udsCtx.totalMemorySize = ((uint32_t)data[6]<<24) | ((uint32_t)data[7]<<16) | ((uint32_t)data[8]<<8) | data[9];
    }
    
    printf("[UDS_34] Addr=0x%08X, Size=%d\r\n", udsCtx.expectedMemoryAddress, udsCtx.totalMemorySize);
    
    udsCtx.flashState = UDS_FLASH_STATE_REQUESTED;
    udsCtx.transferredSize = 0;
    udsCtx.blockSequenceCounter = 1; 
    
    txBuf[0] = 0x74;        
    txBuf[1] = 0x20;        
    txBuf[2] = (TP_MAX_BUF_SIZE >> 8) & 0xFF; 
    txBuf[3] = TP_MAX_BUF_SIZE & 0xFF;        
    TpSendSingleFrame(txBuf, 4);
}

/**
  * @brief      处理 0x36 传输数据 (Transfer Data)
  * @details    连续接收固件数据包，校验块序列号并累计已传输字节数。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @note       进入此函数时，已确保处于编程会话且安全解锁。
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService36(uint8_t *data, uint16_t len) {
    uint8_t blockSeqCounter;
    uint8_t txBuf[2];
    
    if(len < 3) {
        UdsSendNegativeResponse(UDS_SID_TRANSFER_DATA, NRC_INCORRECT_MESSAGE_LENGTH);
        return;
    }
    
    blockSeqCounter = data[1];
    printf("[UDS_36] RX 0x36 Block %d\r\n", blockSeqCounter);
    
    if((uint8_t)(udsCtx.blockSequenceCounter & 0xFF) != blockSeqCounter) {
        UdsSendNegativeResponse(UDS_SID_TRANSFER_DATA, 0x73);
        return;
    }
    
    udsCtx.flashState = UDS_FLASH_STATE_TRANSFERING;
    udsCtx.transferredSize += (len - 2);
    udsCtx.blockSequenceCounter++; 
    
    txBuf[0] = 0x76;       
    txBuf[1] = blockSeqCounter;
    TpSendSingleFrame(txBuf, 2);
}

/**
  * @brief      处理 0x37 请求退出传输 (Request Transfer Exit)
  * @details    测试仪表明固件刷写完成，复位刷写上下文并回复肯定响应。
  * @param[in]  data UDS Payload 指针
  * @param[in]  len  数据包长度
  * @return     无
  * @note       进入此函数时，已确保处于编程会话且安全解锁。
  * @author     小昭debug
  * @date       2026-02-26
  */
static void UdsHandleService37(uint8_t *data, uint16_t len) {
    uint8_t txBuf[1];
    
    printf("[UDS_37] RX 0x37 Request Transfer Exit\r\n");
    
    udsCtx.flashState = UDS_FLASH_STATE_IDLE;
    udsCtx.expectedMemoryAddress = 0;
    udsCtx.totalMemorySize = 0;

    txBuf[0] = 0x77;       
    TpSendSingleFrame(txBuf, 1);
}
