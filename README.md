# UDS (Unified Diagnostic Services) Protocol Stack

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ARM%20Cortex%20M4-orange.svg)]()
[![Standard](https://img.shields.io/badge/standard-ISO%2014229--1%20%7C%20ISO%2015765--2-green.svg)]()

🌐 **English** | [中文](README_ZH.md)

A lightweight, portable **UDS Diagnostic Protocol Stack** implementation based on ISO 14229-1 and ISO 15765-2 standards, supporting both Classic CAN and CAN FD.

## 📁 Project Structure

```
publilc_code/
├── 03 CANFD_Origin_Release/          # NXP S32K144 Platform (CAN FD)
│   ├── Sources/
│   │   ├── main.c                    # Main entry point
│   │   ├── can_fd_wrapper.c/.h       # CAN FD hardware abstraction layer
│   │   └── uds/                      # UDS protocol stack core
│   │       ├── uds_core.h/.c         # Core layer (initialization, message dispatch)
│   │       ├── uds_tp.h/.c           # CAN TP transport layer
│   │       ├── uds_session.h/.c      # Session management
│   │       ├── uds_security.h/.c     # Security access management
│   │       ├── uds_routine.h/.c      # Routine control
│   │       ├── uds_pending.h/.c      # 0x78 ResponsePending
│   │       ├── uds_nrc.h/.c          # Negative response handling
│   │       ├── services/             # UDS service implementations
│   │       │   ├── uds_svc_10.h/.c   # Diagnostic Session Control (0x10)
│   │       │   ├── uds_svc_22.h/.c   # Read Data By Identifier (0x22)
│   │       │   ├── uds_svc_27.h/.c   # Security Access (0x27)
│   │       │   └── uds_svc_31.h/.c   # Routine Control (0x31)
│   │       └── UDS_Design_Spec.md    # Detailed design specification
│   └── SDK/                          # NXP S32K SDK
│
└── UDS_CAN_STM32F4/                  # STM32F4 Platform (Classic CAN)
    ├── Main/
    ├── USER/
    │   ├── CAN/                      # CAN driver
    │   ├── UDS/                      # UDS application layer
    │   │   ├── iso15765_tp.h/.c      # ISO 15765-2 TP layer
    │   │   ├── uds_app.h/.c          # UDS application layer
    │   ├── KEY/                      # Key/button driver
    │   └── LCD/                      # LCD display driver
    ├── STM32F4_FWLIB/                # STM32F4 Standard Peripheral Library
    └── Project/                      # Keil project files
```

## ✨ Key Features

### 🔧 Protocol Support
- **ISO 14229-1 (UDS)** - Unified Diagnostic Services standard
- **ISO 15765-2 (CAN TP)** - CAN transport layer protocol
- **Classic CAN** (8-byte data frames)
- **CAN FD** (64-byte data frames)
- **Multi-frame transmission** (up to 4095 bytes)

### 🚀 Implemented Services (SID)
| SID | Service Name | Description |
|-----|--------------|-------------|
| 0x10 | Diagnostic Session Control | Session management (Default/Extended/Programming) |
| 0x22 | Read Data By Identifier | Read data by DID |
| 0x27 | Security Access | Seed/key security mechanism |
| 0x31 | Routine Control | Start/Stop/Request routine results |
| 0x3E | Tester Present | Keep diagnostic session alive |

### 🛡️ Security Features
- **Bitmap-based session permission control**
- **Multi-level security access** management
- Seed-key algorithm (user-customizable)
- Attempt limit and delay lock mechanism

### ⚡ Advanced Features
- **Physical** and **Functional** addressing support
- **Suppress Positive Response (SPR)** bit handling
- **0x78 ResponsePending** mechanism
- **Table-driven configuration** architecture (easy to extend)
- Complete **NRC (Negative Response Code)** support

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                      │
│                   (User-defined diagnostic services)         │
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
│              (Platform-specific: S32K144 / STM32F4)         │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### 1. Clone Repository
```bash
git clone https://github.com/xiaozhaodebug/public_code.git
cd public_code
```

### 2. Select Platform
- **NXP S32K144**: Open S32DS project in `03 CANFD_Origin_Release/`
- **STM32F4**: Open Keil project in `UDS_CAN_STM32F4/Project/`

### 3. Basic Usage

#### Initialize UDS Stack
```c
#include "uds_stack.h"

void UdsStackInit(void)
{
    /* Initialize TP layer */
    UdsTpInit();
    
    /* Initialize session management */
    UdsSessionInit();
    
    /* Initialize security access */
    UdsSecurityInit();
    
    /* Initialize UDS core */
    UdsInit(gUdsServiceTable, serviceCount);
}
```

#### CAN RX Interrupt Handler
```c
void CAN_Rx_IRQHandler(void)
{
    CanMessage rxMsg;
    CAN_Receive(&rxMsg);
    
    /* Call UDS receive indication */
    UdsCanRxIndication(rxMsg.id, rxMsg.data, rxMsg.length);
}
```

#### 1ms Timer Interrupt
```c
void TIM_IRQHandler(void)
{
    /* UDS timer handling */
    UdsTimerISR();
}
```

#### Main Loop
```c
int main(void)
{
    SystemInit();
    UdsStackInit();
    
    while(1)
    {
        /* UDS main task */
        UdsMainTask();
    }
}
```

## ⚙️ Configuration

### CAN ID Configuration
| Type | ID (Hex) | Description |
|------|----------|-------------|
| Physical Request | 0x74C | ECU receives diagnostic requests |
| Physical Response | 0x75C | ECU sends diagnostic responses |
| Functional | 0x7DF | Broadcast requests |

### Timing Parameters (Configurable)
```c
/* CAN TP layer timing */
#define UDS_TP_N_AR          1000u   /* Receiver response timeout (ms) */
#define UDS_TP_N_CR          1000u   /* Consecutive frame wait (ms) */
#define UDS_TP_ST_MIN        20u     /* Minimum separation time (ms) */

/* UDS layer timing */
#define UDS_P2_SERVER_DEFAULT    50u      /* Default response timeout (ms) */
#define UDS_P2_SERVER_EXTENDED   5000u    /* Extended response timeout (ms) */
#define UDS_S3_SERVER_TIMEOUT    5000u    /* Session timeout (ms) */
```

## 📚 Documentation

- [UDS Design Specification](03%20CANFD_Origin_Release/Sources/uds/UDS_Design_Spec.md) - Detailed architecture and API documentation (Chinese)

## 🛠️ Development Environment

| Platform | IDE | Compiler |
|----------|-----|----------|
| S32K144 | NXP S32 Design Studio | GCC ARM Embedded |
| STM32F4 | Keil MDK-ARM | ARM Compiler 5/6 |

## 🤝 Contributing

Issues and Pull Requests are welcome!

## 📝 License

This project is open-sourced under the [MIT](LICENSE) license.

## 👨‍💻 Author

- **xiaozhaodebug** - [xiaozhaodebug](https://github.com/xiaozhaodebug)

---

> 💡 **Note**: This is an educational UDS protocol stack implementation, suitable for learning UDS protocol principles and embedded diagnostic development.
