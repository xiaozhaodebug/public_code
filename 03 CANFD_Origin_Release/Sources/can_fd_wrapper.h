/**
 * @file can_fd_wrapper.h
 * @brief CAN FD 发送报文封装层头文件
 * @note 采用驼峰命名风格
 */

#ifndef CAN_FD_WRAPPER_H
#define CAN_FD_WRAPPER_H

#include "can_pal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LED1(x)  PINS_DRV_WritePin(PTD,16,!x);
#define LED2(x)  PINS_DRV_WritePin(PTD,15,!x);
#define LED3(x)  PINS_DRV_WritePin(PTD,1,!x);
#define LED4(x)  PINS_DRV_WritePin(PTD,0,!x);

#define Rx_Filter  0x0


#define RX_MAILBOX_CAN0  (0UL)
#define TX_MAILBOX_CAN0  (1UL)

#define RX_MAILBOX_CAN1  (2UL)
#define TX_MAILBOX_CAN1  (3UL)

#define RX_MAILBOX_CAN2  (4UL)
#define TX_MAILBOX_CAN2  (5UL)

extern char IRQ_CAN0_RX;

extern can_message_t recvMsg_CAN0;

/**
 * @brief CAN FD 报文配置结构体
 */
typedef struct {
    uint32_t msgId;          /*!< 报文ID */
    bool isExtendedId;       /*!< 是否为扩展ID (29位) */
    uint8_t data[64];        /*!< 报文数据 */
    uint8_t dataLength;      /*!< 数据长度 (0-64) */
    bool enableBrs;          /*!< 是否启用比特率切换 (BRS) */
} CanFdMessageConfig;

/**
 * @brief 发送 CAN FD 报文（非阻塞方式）
 *
 * @param[in] instance CAN 实例指针
 * @param[in] mailboxIdx 发送邮箱索引
 * @param[in] config 报文配置结构体指针
 * @return status_t 发送状态
 *         - STATUS_SUCCESS: 发送成功
 *         - STATUS_BUSY: 邮箱正忙
 *         - STATUS_ERROR: 其他错误
 */
status_t CanFdSendMessage(const can_instance_t *const instance,
                          uint32_t mailboxIdx,
                          const CanFdMessageConfig *config);

/**
 * @brief 发送 CAN FD 报文（阻塞方式）
 *
 * @param[in] instance CAN 实例指针
 * @param[in] mailboxIdx 发送邮箱索引
 * @param[in] config 报文配置结构体指针
 * @param[in] timeoutMs 超时时间（毫秒）
 * @return status_t 发送状态
 *         - STATUS_SUCCESS: 发送成功
 *         - STATUS_TIMEOUT: 发送超时
 *         - STATUS_ERROR: 其他错误
 */
status_t CanFdSendMessageBlocking(const can_instance_t *const instance,
                                  uint32_t mailboxIdx,
                                  const CanFdMessageConfig *config,
                                  uint32_t timeoutMs);

/**
 * @brief 快速发送 CAN FD 报文（简化接口）
 *
 * @param[in] instance CAN 实例指针
 * @param[in] mailboxIdx 发送邮箱索引
 * @param[in] msgId 报文ID
 * @param[in] data 数据指针
 * @param[in] dataLength 数据长度 (0-64)
 * @return status_t 发送状态
 */
status_t CanFdSendQuick(const can_instance_t *const instance,
                        uint32_t mailboxIdx,
                        uint32_t msgId,
                        const uint8_t *data,
                        uint8_t dataLength);

/**
 * @brief 配置 CAN FD 发送邮箱
 *
 * @param[in] instance CAN 实例指针
 * @param[in] mailboxIdx 发送邮箱索引
 * @param[in] enableBrs 是否启用比特率切换
 * @param[in] isExtendedId 是否使用扩展ID
 * @return status_t 配置状态
 */
status_t CanFdConfigTxMailbox(const can_instance_t *const instance,
                              uint32_t mailboxIdx,
                              bool enableBrs,
                              bool isExtendedId);


                              void CAN0_Init(void);


#ifdef __cplusplus
}
#endif

#endif /* CAN_FD_WRAPPER_H */
