# UDS (Unified Diagnostic Services) Protocol Stack

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

🌐 **中文** | [English](README.md)
[![Platform](https://img.shields.io/badge/platform-ARM%20Cortex%20M4-orange.svg)]()
[![Standard](https://img.shields.io/badge/standard-ISO%2014229--1%20%7C%20ISO%2015765--2-green.svg)]()

一个轻量级、可移植的 **UDS 诊断协议栈**实现，基于 ISO 14229-1 和 ISO 15765-2 标准，支持经典 CAN 和 CAN FD。

## 📁 项目结构

```
publilc_code/
├── 03 CANFD_Origin_Release/          # NXP S32K144 平台实现 (CAN FD)
│   ├── Sources/
│   │   ├── main.c                    # 主程序入口
│   │   ├── can_fd_wrapper.c/.h       # CAN FD 硬件抽象层
│   │   └── uds/                      # UDS 协议栈核心
│   │       ├── uds_core.c/.h         # 核心层（初始化、消息分发）
│   │       ├── uds_tp.c/.h           # CAN TP 传输层
│   │       ├── uds_session.c/.h      # 会话管理
│   │       ├── uds_security.c/.h     # 安全访问管理
│   │       ├── uds_routine.c/.h      # 例程控制
│   │       ├── uds_pending.c/.h      # 0x78 ResponsePending
│   │       ├── uds_nrc.c/.h          # 否定响应处理
│   │       ├── services/             # UDS 服务实现
│   │       │   ├── uds_svc_10.c/.h   # 诊断会话控制 (0x10)
│   │       │   ├── uds_svc_22.c/.h   # 通过ID读取数据 (0x22)
│   │       │   ├── uds_svc_27.c/.h   # 安全访问 (0x27)
│   │       │   └── uds_svc_31.c/.h   # 例程控制 (0x31)
│   │       └── UDS_Design_Spec.md    # 详细设计规格书
│   └── SDK/                          # NXP S32K SDK
│
└── UDS_CAN_STM32F4/                  # STM32F4 平台实现 (经典 CAN)
    ├── Main/
    ├── USER/
    │   ├── CAN/                      # CAN 驱动
    │   ├── UDS/                      # UDS 应用层
    │   │   ├── iso15765_tp.c/.h      # ISO 15765-2 TP 层
    │   │   ├── uds_app.c/.h          # UDS 应用层
    │   ├── KEY/                      # 按键驱动
    │   └── LCD/                      # LCD 显示驱动
    ├── STM32F4_FWLIB/                # STM32F4 标准库
    └── Project/                      # Keil 工程文件
```

## ✨ 核心特性

### 🔧 协议支持
- **ISO 14229-1 (UDS)** - 统一的诊断服务标准
- **ISO 15765-2 (CAN TP)** - CAN 传输层协议
- **经典 CAN** (8 字节数据帧)
- **CAN FD** (64 字节数据帧)
- **多帧传输** (最大 4095 字节)

### 🚀 已实现的服务 (SID)
| SID | 服务名称 | 说明 |
|-----|----------|------|
| 0x10 | Diagnostic Session Control | 诊断会话控制（默认/扩展/编程会话） |
| 0x22 | Read Data By Identifier | 通过 ID 读取数据 |
| 0x27 | Security Access | 安全访问（种子/密钥机制） |
| 0x31 | Routine Control | 例程控制（启动/停止/请求结果） |
| 0x3E | Tester Present | 测试仪在线保持 |

### 🛡️ 安全特性
- 基于位图的**会话权限控制**
- **安全访问等级**管理（支持多级解锁）
- 种子-密钥算法（用户可自定义）
- 尝试次数限制和延时锁定机制

### ⚡ 高级功能
- **物理寻址**和**功能寻址**支持
- **抑制肯定响应 (SPR)** 位处理
- **0x78 ResponsePending** 机制
- **配置表驱动**架构（易于扩展新服务）
- 完整的 **NRC (否定响应码)** 支持

## 🏗️ 架构设计

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
│  │  Dispatcher │ │   Manager   │ │       Manager        │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      CAN TP Layer                           │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │ Single Frame│ │ First Frame │ │  Consecutive Frame   │  │
│  │    (SF)     │ │    (FF)     │ │        (CF)          │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
│  ┌─────────────┐ ┌─────────────┐                           │
│  │  Flow Ctrl  │ │  Multi-frame│                           │
│  │    (FC)     │ │   Buffer    │                           │
│  └─────────────┘ └─────────────┘                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    CAN Driver Layer                         │
│              (平台相关: S32K144 / STM32F4)                  │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 快速开始

### 1. 克隆仓库
```bash
git clone https://github.com/xiaozhaodebug/public_code.git
cd public_code
```

### 2. 选择平台
- **NXP S32K144**: 打开 `03 CANFD_Origin_Release/` 目录下的 S32DS 工程
- **STM32F4**: 打开 `UDS_CAN_STM32F4/Project/` 目录下的 Keil 工程

### 3. 基本使用

#### 初始化 UDS 协议栈
```c
#include "uds_stack.h"

void UdsStackInit(void)
{
    /* 初始化 TP 层 */
    UdsTpInit();
    
    /* 初始化会话管理 */
    UdsSessionInit();
    
    /* 初始化安全访问 */
    UdsSecurityInit();
    
    /* 初始化 UDS 核心 */
    UdsInit(gUdsServiceTable, serviceCount);
}
```

#### CAN 接收中断处理
```c
void CAN_Rx_IRQHandler(void)
{
    CanMessage rxMsg;
    CAN_Receive(&rxMsg);
    
    /* 调用 UDS 接收指示 */
    UdsCanRxIndication(rxMsg.id, rxMsg.data, rxMsg.length);
}
```

#### 1ms 定时器中断
```c
void TIM_IRQHandler(void)
{
    /* UDS 定时器处理 */
    UdsTimerISR();
}
```

#### 主循环
```c
int main(void)
{
    SystemInit();
    UdsStackInit();
    
    while(1)
    {
        /* UDS 主任务处理 */
        UdsMainTask();
    }
}
```

## ⚙️ 配置说明

### CAN ID 配置
| 类型 | ID (Hex) | 说明 |
|------|----------|------|
| 物理请求地址 | 0x74C | ECU 接收诊断请求 |
| 物理响应地址 | 0x75C | ECU 发送诊断响应 |
| 功能寻址 | 0x7DF | 广播请求 |

### 时间参数 (可配置)
```c
/* CAN TP 层时间参数 */
#define UDS_TP_N_AR          1000u   /* 接收方响应时间 (ms) */
#define UDS_TP_N_CR          1000u   /* 接收方连续帧等待 (ms) */
#define UDS_TP_ST_MIN        20u     /* 默认最小间隔 (ms) */

/* UDS 层时间参数 */
#define UDS_P2_SERVER_DEFAULT    50u      /* 默认响应超时 (ms) */
#define UDS_P2_SERVER_EXTENDED   5000u    /* 扩展响应超时 (ms) */
#define UDS_S3_SERVER_TIMEOUT    5000u    /* 会话超时 (ms) */
```

## 📚 文档

- [UDS 设计规格书](03%20CANFD_Origin_Release/Sources/uds/UDS_Design_Spec.md) - 详细的架构设计和 API 文档

## 🛠️ 开发环境

| 平台 | IDE | 编译器 |
|------|-----|--------|
| S32K144 | NXP S32 Design Studio | GCC ARM Embedded |
| STM32F4 | Keil MDK-ARM | ARM Compiler 5/6 |

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📝 许可证

本项目基于 [MIT](LICENSE) 许可证开源。

## 👨‍💻 作者

- **小昭debug** - [xiaozhaodebug](https://github.com/xiaozhaodebug)

---

> 💡 **提示**: 这是一个教育性质的 UDS 协议栈实现，适合学习 UDS 协议原理和嵌入式诊断开发。
