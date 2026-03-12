/**
 * @file uds_stack.h
 * @brief UDS协议栈总入口头文件
 * @note 用户只需包含此文件即可使用UDS协议栈
 */

#ifndef UDS_STACK_H
#define UDS_STACK_H

/* 核心头文件 */
#include "uds_config.h"
#include "uds_types.h"
#include "uds_platform.h"
#include "uds_core.h"

/* 各模块头文件 */
#include "uds_tp.h"
#include "uds_session.h"
#include "uds_security.h"
#include "uds_routine.h"
#include "uds_pending.h"
#include "uds_nrc.h"

/* 服务实现头文件 */
#include "services/uds_svc_10.h"
#include "services/uds_svc_27.h"
#include "services/uds_svc_31.h"

#endif /* UDS_STACK_H */
