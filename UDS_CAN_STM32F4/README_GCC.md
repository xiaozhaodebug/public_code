# STM32F407 UDS CAN 诊断系统

基于 STM32F4 标准库实现的 UDS (Unified Diagnostic Services) 诊断通信系统，支持 CAN 总线通信和 ISO-TP 传输协议。

## 功能特性

- **UDS 诊断服务**
  - 0x10 诊断会话控制（默认/编程/扩展会话）
  - 0x22 读DID数据
  - 0x27 安全访问服务
  - 0x34/0x36/0x37 固件刷写服务（请求下载/传输数据/退出传输）
  - 0x3E 测试仪在线保持

- **通信协议**
  - CAN 总线通信（500Kbps，可配置）
  - ISO-TP (ISO 15765-2) 传输层协议
  - 支持单帧和多帧传输

- **硬件支持**
  - STM32F407VG 微控制器
  - 双 CAN 接口（CAN1/CAN2）
  - LCD 显示、LED 指示、按键输入

## 项目结构

```
├── Main/                   # 主程序入口
├── Startup_config/         # 启动文件和系统配置
├── STM32F4_FWLIB/          # STM32F4 标准库
├── Common/                 # 通用工具函数（延时等）
└── USER/
    ├── CAN/                # CAN 驱动
    ├── UDS/                # UDS 诊断协议实现
    ├── LCD/                # LCD 显示驱动
    ├── LED/                # LED 驱动
    ├── KEY/                # 按键驱动
    └── usart1/             # 串口调试
```

## 编译说明

### 支持的编译工具
- GCC ARM Toolchain（已配置 Makefile）
- Keil MDK（原工程文件 CAN.uvprojx）

### GCC 编译步骤

```bash
# 1. 进入项目目录
cd UDS_CAN_STM32F4

# 2. 编译
make clean
make -j4

# 3. 输出文件
#   build/uds_can_stm32f4.hex  - Intel HEX 格式
#   build/uds_can_stm32f4.bin  - 二进制格式
```

### 烧录方法

#### 使用 JLink
```bash
JLinkExe -device STM32F407VG -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
```

#### 使用 OpenOCD
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/uds_can_stm32f4.bin 0x08000000 verify reset exit"
```

## 硬件连接

### CAN 总线连接
- CAN1_TX  -> PA12
- CAN1_RX  -> PA11
- CAN2_TX  -> PB13
- CAN2_RX  -> PB12

### 其他外设
- LCD      -> 根据具体型号接线
- LED0     -> PF9
- LED1     -> PF10
- KEY0     -> PA0
- USART1   -> PA9(TX), PA10(RX)

## UDS 通信示例

### 使用 SocketCAN (Linux)

```bash
# 配置 CAN 接口
sudo ip link set can0 type can bitrate 500000
sudo ip link set up can0

# 发送诊断请求（进入默认会话）
cansend can0 7E0#0210010000000000

# 监听响应
candump can0
```

### 使用 Python-can

```python
import can

bus = can.interface.Bus(interface='socketcan', channel='can0', bitrate=500000)

# 发送 10 01 进入默认会话
msg = can.Message(arbitration_id=0x7E0, data=[0x02, 0x10, 0x01], is_extended_id=False)
bus.send(msg)

# 接收响应
response = bus.recv(timeout=1.0)
if response:
    print(f"RX: {response.data.hex()}")

bus.shutdown()
```

## 内存占用

编译后固件大小：
- Flash: ~12KB (text + data)
- RAM: ~3KB (bss)

## 依赖

- ARM GCC Toolchain 10.3+ / 13.x
- STM32F4xx 标准库
- python-can (可选，用于测试)

## 协议参考

- ISO 14229 (UDS)
- ISO 15765-2 (CAN 上的传输协议)
- ISO 11898 (CAN 总线)

## 更新日志

- 初始版本：基于 STM32F4 标准库实现基础 UDS 服务
- GCC 支持：添加 Makefile 和 GCC 启动文件
