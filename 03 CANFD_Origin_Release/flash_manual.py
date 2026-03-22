#!/usr/bin/env python3
"""
S32K144 手动烧录脚本
由于 J-Link loadbin 命令有问题，使用 w4 命令逐字写入
"""

import subprocess
import struct
import sys

def create_jlink_script(bin_file, start_addr=0x00000000):
    """创建 J-Link 脚本"""
    with open(bin_file, 'rb') as f:
        data = f.read()
    
    script_lines = [
        "device S32K144",
        "si swd",
        "speed 2000",
        "connect",
        "h",
        "erase",
    ]
    
    # 每行写入 16 字节 (4 个 32-bit 字)
    addr = start_addr
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        # 补齐到 16 字节
        if len(chunk) < 16:
            chunk = chunk + b'\xFF' * (16 - len(chunk))
        
        # 写入 4 个 32-bit 字
        for j in range(0, 16, 4):
            word = struct.unpack('<I', chunk[j:j+4])[0]
            script_lines.append(f"w4 0x{addr+j:08X}, 0x{word:08X}")
        
        addr += 16
        
        # 每 1KB 显示进度
        if (addr - start_addr) % 1024 == 0:
            script_lines.append(f'print "Written {addr - start_addr} bytes..."')
    
    script_lines.extend([
        "r",
        "g",
        "qc"
    ])
    
    return '\n'.join(script_lines)

if __name__ == '__main__':
    bin_file = "build/S32K144_CANFD_UDS.bin"
    script = create_jlink_script(bin_file)
    
    with open("flash_manual.jlink", "w") as f:
        f.write(script)
    
    print("Created flash_manual.jlink")
    print("Running JLinkExe...")
    
    result = subprocess.run(
        ["JLinkExe", "-CommanderScript", "flash_manual.jlink"],
        capture_output=True,
        text=True
    )
    
    print(result.stdout)
    if result.returncode != 0:
        print("Error:", result.stderr)
        sys.exit(1)
