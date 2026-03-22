# S32K144 CAN FD UDS 工程编译指南

## 编译环境

- **工具链**: ARM GCC (arm-none-eabi-gcc)
- **构建系统**: CMake
- **MCU**: NXP S32K144 (ARM Cortex-M4F)
- **Flash 大小**: 512KB
- **RAM 大小**: 64KB

## 快速开始 (使用 Skill 脚本)

```bash
cd /home/claw/share/AUTOSAR_Easy/pro/public_code/03\ CANFD_Origin_Release

# 编译工程
.k compile-s32k144

# 烧录固件
.k flash-s32k144

# 编译+烧录一键完成
.k build-and-flash-s32k144

# 验证 Flash
.k verify-s32k144
```

## 手动编译步骤

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi cmake build-essential

# 检查安装
arm-none-eabi-gcc --version
```

### 2. 编译工程

```bash
cd /home/claw/share/AUTOSAR_Easy/pro/public_code/03\ CANFD_Origin_Release

# 创建构建目录
mkdir -p build && cd build

# 配置
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake ..

# 编译
make -j$(nproc)
```

### 3. 编译输出

编译成功后，会生成以下文件：

| 文件 | 说明 | 大小 |
|------|------|------|
| `S32K144_CANFD_UDS.elf` | ELF 调试文件 | ~167KB |
| `S32K144_CANFD_UDS.hex` | Intel HEX 格式 | ~144KB |
| `S32K144_CANFD_UDS.bin` | 二进制格式 | ~51KB |
| `S32K144_CANFD_UDS.map` | 内存映射文件 | - |

### 4. 内存使用

```
   text    data     bss     dec     hex filename
  50136    1096   12608   63840    f960 S32K144_CANFD_UDS.elf
```

- **Flash**: ~51KB (text + data) / 512KB = 约 10%
- **RAM**: ~13.6KB (data + bss) / 64KB = 约 21%

## 烧录固件

### 方式 1: 使用 Skill 脚本 (推荐)

```bash
cd /home/claw/share/AUTOSAR_Easy/pro/public_code/03\ CANFD_Origin_Release

# 编译并烧录
.k build-and-flash-s32k144

# 或仅烧录现有固件
.k flash-s32k144
```

### 方式 2: 使用 JLink 手动烧录

#### 前提条件
- 安装 J-Link 软件: https://www.segger.com/downloads/jlink/
- 确保 JLinkExe 在 PATH 中

#### 烧录命令

```bash
# 使用 Python 烧录脚本 (逐字写入)
cd /home/claw/share/AUTOSAR_Easy/pro/public_code/03\ CANFD_Origin_Release
python3 /home/claw/share/AUTOSAR_Easy/.kimi/skills/s32k144-compile-flash/scripts/flash_s32k144.py

# 或使用 CMake 目标
cd build
make flash
```

### 硬件连接

| JTAG/SWD 引脚 | S32K144 引脚 |
|---------------|--------------|
| SWDIO/TMS | PTA3 |
| SWCLK/TCK | PTA0 |
| RESET | RESET |
| GND | GND |
| VCC | 3.3V |

## CAN 测试功能

固件包含每秒发送 CAN 报文的功能：

| 参数 | 值 |
|------|-----|
| CAN ID | 0x301 |
| 数据 | AA AA AA AA AA AA AA AA |
| 周期 | 1 秒 |
| 波特率 | 500 Kbps |

使用 CAN 设备可接收到该测试报文。

## 工程结构

```
.
├── CMakeLists.txt          # CMake 主配置文件
├── cmake/
│   └── toolchain.cmake     # ARM GCC 工具链配置
├── include/                # 头文件
├── Sources/                # 源文件
│   ├── main.c              # 主程序
│   ├── can_fd_wrapper.c    # CAN FD 封装
│   └── uds/                # UDS 协议栈
│       ├── uds_core.c      # UDS 核心
│       ├── uds_tp.c        # CAN TP 层
│       ├── uds_session.c   # 会话管理
│       ├── uds_security.c  # 安全访问
│       └── services/       # UDS 服务实现
├── SDK/                    # S32K SDK
├── Generated_Code/         # Processor Expert 生成代码
├── build/                  # 构建输出目录
└── BUILD.md               # 本文件
```

## 清理构建

```bash
cd build
make clean        # 清理编译输出
rm -rf build      # 完全删除构建目录
```

## 常见问题

### 1. 找不到 arm-none-eabi-gcc

```bash
sudo apt-get install gcc-arm-none-eabi
```

### 2. 找不到 JLinkExe

下载并安装 J-Link 软件包：
https://www.segger.com/downloads/jlink/

### 3. 烧录失败

- 检查 JTAG/SWD 连接
- 检查目标板电源
- 尝试降低 SWD 速度
- 确保 J-Link 固件已更新

## UDS 功能

编译后的固件支持以下 UDS 服务：

| SID | 服务 | 说明 |
|-----|------|------|
| 0x10 | DiagnosticSessionControl | 诊断会话控制 |
| 0x22 | ReadDataByIdentifier | 根据标识符读取数据 |
| 0x27 | SecurityAccess | 安全访问 |
| 0x31 | RoutineControl | 例程控制 |

CAN ID 配置：
- 请求: 0x74C
- 响应: 0x75C
- 功能寻址: 0x7DF

## 相关 Skill

- `s32k144-compile-flash` - S32K144 编译烧录工具
- `zm-canfd` - CAN 通信测试
