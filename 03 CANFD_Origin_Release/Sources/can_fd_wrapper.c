/**
 * @file can_fd_wrapper.c
 * @brief CAN FD 发送报文封装层实现
 * @note 采用驼峰命名风格
 */

#include "can_fd_wrapper.h"
#include <string.h>
#include "Cpu.h"
#include "uds_stack.h"

char IRQ_CAN0_RX;

can_message_t recvMsg_CAN0;


can_buff_config_t Rx_buffCfg =  {
    .enableFD = true,
    .enableBRS = false,
    .fdPadding = 0U,
    .idType = CAN_MSG_ID_STD,
    .isRemote = false
};

can_buff_config_t Tx_buffCfg =  {
    .enableFD = true,
    .enableBRS = false,
    .fdPadding = 0U,
    .idType = CAN_MSG_ID_STD,
    .isRemote = false
};


/**
 * @brief 将数据长度转换为 CAN FD DLC 值
 * @param[in] dataLength 数据长度 (0-64)
 * @return uint8_t DLC 值
 */
static uint8_t CanFdConvertLengthToDlc(uint8_t dataLength)
{
    if (dataLength <= 8U) {
        return dataLength;
    } else if (dataLength <= 12U) {
        return 9U;
    } else if (dataLength <= 16U) {
        return 10U;
    } else if (dataLength <= 20U) {
        return 11U;
    } else if (dataLength <= 24U) {
        return 12U;
    } else if (dataLength <= 32U) {
        return 13U;
    } else if (dataLength <= 48U) {
        return 14U;
    } else {
        return 15U;
    }
}

/**
 * @brief 将 DLC 转换为实际数据长度
 * @param[in] dlc DLC 值
 * @return uint8_t 实际数据长度
 */
static uint8_t CanFdConvertDlcToLength(uint8_t dlc)
{
    const uint8_t dlcToLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 
                                   12U, 16U, 20U, 24U, 32U, 48U, 64U};
    if (dlc <= 15U) {
        return dlcToLength[dlc];
    }
    return 64U;
}

status_t CanFdSendMessage(const can_instance_t *const instance,
                          uint32_t mailboxIdx,
                          const CanFdMessageConfig *config)
{
    if (instance == NULL || config == NULL) {
        return STATUS_ERROR;
    }

    /* 限制数据长度在有效范围内 */
    uint8_t actualLength = config->dataLength;
    if (actualLength > 64U) {
        actualLength = 64U;
    }

    /* 构建 CAN 报文结构 */
    can_message_t txMessage;
    txMessage.cs = 0U;
    txMessage.id = config->msgId;
    txMessage.length = CanFdConvertDlcToLength(CanFdConvertLengthToDlc(actualLength));

    /* 复制数据 */
    if (actualLength > 0U && config->data != NULL) {
        (void)memcpy(txMessage.data, config->data, actualLength);
    }

    /* 剩余空间填充 0 */
    if (actualLength < txMessage.length) {
        (void)memset(&txMessage.data[actualLength], 0U, txMessage.length - actualLength);
    }

    return CAN_Send(instance, mailboxIdx, &txMessage);
}

status_t CanFdSendMessageBlocking(const can_instance_t *const instance,
                                  uint32_t mailboxIdx,
                                  const CanFdMessageConfig *config,
                                  uint32_t timeoutMs)
{
    if (instance == NULL || config == NULL) {
        return STATUS_ERROR;
    }

    /* 限制数据长度在有效范围内 */
    uint8_t actualLength = config->dataLength;
    if (actualLength > 64U) {
        actualLength = 64U;
    }

    /* 构建 CAN 报文结构 */
    can_message_t txMessage;
    txMessage.cs = 0U;
    txMessage.id = config->msgId;
    txMessage.length = CanFdConvertDlcToLength(CanFdConvertLengthToDlc(actualLength));

    /* 复制数据 */
    if (actualLength > 0U && config->data != NULL) {
        (void)memcpy(txMessage.data, config->data, actualLength);
    }

    /* 剩余空间填充 0 */
    if (actualLength < txMessage.length) {
        (void)memset(&txMessage.data[actualLength], 0U, txMessage.length - actualLength);
    }

    return CAN_SendBlocking(instance, mailboxIdx, &txMessage, timeoutMs);
}

status_t CanFdSendQuick(const can_instance_t *const instance,
                        uint32_t mailboxIdx,
                        uint32_t msgId,
                        const uint8_t *data,
                        uint8_t dataLength)
{
    if (instance == NULL) {
        return STATUS_ERROR;
    }

    /* 限制数据长度在有效范围内 */
    uint8_t actualLength = dataLength;
    if (actualLength > 64U) {
        actualLength = 64U;
    }

    /* 构建 CAN 报文结构 */
    can_message_t txMessage;
    txMessage.cs = 0U;
    txMessage.id = msgId;
    txMessage.length = CanFdConvertDlcToLength(CanFdConvertLengthToDlc(actualLength));

    /* 复制数据 */
    if (actualLength > 0U && data != NULL) {
        (void)memcpy(txMessage.data, data, actualLength);
    }

    /* 剩余空间填充 0 */
    if (actualLength < txMessage.length) {
        (void)memset(&txMessage.data[actualLength], 0U, txMessage.length - actualLength);
    }
    CAN_ConfigTxBuff(&can_pal0_instance, TX_MAILBOX_CAN0, &Tx_buffCfg); //配置发送
    return CAN_Send(instance, mailboxIdx, &txMessage);
}

status_t CanFdConfigTxMailbox(const can_instance_t *const instance,
                              uint32_t mailboxIdx,
                              bool enableBrs,
                              bool isExtendedId)
{
    if (instance == NULL) {
        return STATUS_ERROR;
    }

    can_buff_config_t buffConfig;
    buffConfig.enableFD = true;           /* CAN FD 模式 */
    buffConfig.enableBRS = enableBrs;     /* 比特率切换 */
    buffConfig.fdPadding = 0U;            /* 填充值 */
    buffConfig.idType = isExtendedId ? CAN_MSG_ID_EXT : CAN_MSG_ID_STD;
    buffConfig.isRemote = false;          /* 非远程帧 */

    return CAN_ConfigTxBuff(instance, mailboxIdx, &buffConfig);
}
uint32_t recv_buff_tick = 0;
/*CAN0回调函数*/
void CAN0_Callback_Func (uint32_t instance,can_event_t event,uint32_t buffIdx,void *flexcanState)
  {
	(void)flexcanState; //此处防止警报
	(void)instance;
	(void)buffIdx;

	switch(event) //回调事件
		{
			case CAN_EVENT_RX_COMPLETE: //接收完成 事件
				CAN_Receive(&can_pal0_instance, RX_MAILBOX_CAN0, &recvMsg_CAN0); //接收报文并重新注册回调函数
				IRQ_CAN0_RX =1;
                recv_buff_tick++;
				/* 调用UDS接收指示，将报文传递给UDS协议栈 */
				UdsCanRxIndication(recvMsg_CAN0.id, recvMsg_CAN0.data, recvMsg_CAN0.length);
				break;
			case CAN_EVENT_TX_COMPLETE: //发送完成事件
				break;
			default:
				break;
		}
  }



void CAN0_Init(void)
{
	  CAN_Init(&can_pal0_instance, &can_pal0_Config0);

	  CAN_ConfigRxBuff(&can_pal0_instance, RX_MAILBOX_CAN0, &Rx_buffCfg, Rx_Filter); //注册接收配置和MSGID过滤器(如过滤器配置为0x1，则只接受msgid 0x1发来的报文)
	  CAN_ConfigTxBuff(&can_pal0_instance, TX_MAILBOX_CAN0, &Tx_buffCfg); //配置发送
	  /*设置MSGID的掩码，掩码粗略可以理解为对11bit MSGID地址的过滤
	   如果某bit位需要过滤设置为1,不过滤设置为0,例如掩码设置为0x7ff则过滤全部标准id,如果设置为0x7fe,这只能接受0x01的报文(不存在0x0的地址)*/
	  CAN_SetRxFilter(&can_pal0_instance,CAN_MSG_ID_STD,RX_MAILBOX_CAN0,0); //设置MSGID掩码，
	  CAN_InstallEventCallback(&can_pal0_instance,&CAN0_Callback_Func,(void*)0); //注册回调函数
	  CAN_Receive(&can_pal0_instance, RX_MAILBOX_CAN0, &recvMsg_CAN0); //*****重点****此函数不只有接收作用 还有续订回调函数的作用.
}
