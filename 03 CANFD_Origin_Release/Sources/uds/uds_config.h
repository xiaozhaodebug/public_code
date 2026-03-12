/**
 * @file uds_config.h
 * @brief UDS协议栈用户配置文件
 * @note 用户可根据项目需求修改此文件
 */

#ifndef UDS_CONFIG_H
#define UDS_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * CAN ID 配置
 *===========================================================================*/
#define UDS_PHYS_REQ_ADDR       0x74Cu   /* 物理请求地址 */
#define UDS_PHYS_RESP_ADDR      0x75Cu   /* 物理响应地址 */
#define UDS_FUNC_REQ_ADDR       0x7DFu   /* 功能寻址请求 */

/*=============================================================================
 * CAN TP 层时间参数 (ISO 15765-2) 单位: ms
 *===========================================================================*/
#define UDS_TP_N_AR             1000u    /* 接收方确认时间 */
#define UDS_TP_N_BR             1000u    /* 接收方等待时间 */
#define UDS_TP_N_CR             1000u    /* 接收方连续帧等待 */
#define UDS_TP_N_AS             1000u    /* 发送方确认时间 */
#define UDS_TP_N_BS             1000u    /* 发送方流控等待 */
#define UDS_TP_N_CS             1000u    /* 发送方连续帧间隔 */
#define UDS_TP_ST_MIN           20u      /* 默认最小间隔 (经典CAN) */
#define UDS_TP_ST_MIN_FD        5u       /* CAN FD 最小间隔 */

/*=============================================================================
 * UDS 层时间参数 (ISO 14229-1) 单位: ms
 *===========================================================================*/
#define UDS_P2_SERVER_DEFAULT   50u      /* 默认P2超时 */
#define UDS_P2_SERVER_EXTENDED  5000u    /* 扩展P2*超时 */
#define UDS_S3_SERVER_TIMEOUT   5000u    /* 会话超时S3 */
#define UDS_P2_STAR_MULTIPLIER  100u     /* P2* = P2 * 100 */

/* 0x78 ResponsePending 参数 */
#define UDS_NRC_78_INTERVAL     2000u    /* 0x78发送间隔 */
#define UDS_NRC_78_MAX_COUNT    3u       /* 最大连续发送次数 */

/*=============================================================================
 * 缓冲区配置
 *===========================================================================*/
#define UDS_TP_MAX_FRAME_SIZE   4095u    /* ISO 15765-2 最大帧大小 */
#define UDS_TP_CAN_FD_ENABLE    1        /* 1=支持CAN FD, 0=仅经典CAN */

/* 经典CAN单帧最大数据长度 (ISO 15765-2) */
#define UDS_TP_SF_MAX_LEN_CLASSIC   7u
/* CAN FD单帧最大数据长度 */
#define UDS_TP_SF_MAX_LEN_FD        62u

/*=============================================================================
 * CAN 发送数据填充配置
 *===========================================================================*/
#define UDS_TX_PADDING_ENABLE       1       /* 1=使能发送填充, 0=禁止填充 */
#define UDS_TX_PADDING_LENGTH       12u      /* 填充目标长度: 8/12/16/20/24/32/48/64 */
#define UDS_TX_PADDING_BYTE         0xAAu   /* 填充字节值 (如 0xAA, 0x55, 0x00) */

/*=============================================================================
 * 安全访问配置
 *===========================================================================*/
#define UDS_SECURITY_SEED_SIZE      4u      /* 种子长度(字节) */
#define UDS_SECURITY_KEY_SIZE       4u      /* 密钥长度(字节) */
#define UDS_SECURITY_MAX_LEVELS     1u      /* 支持的最大安全等级数 */

#ifdef __cplusplus
}
#endif

#endif /* UDS_CONFIG_H */
