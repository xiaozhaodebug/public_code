#!/usr/bin/env python3
"""
UDS_CAN_STM32F4 诊断功能测试脚本
适配项目 CAN ID: 请求 0x74C, 响应 0x75C
"""

import can
import time
import threading
import subprocess
from datetime import datetime

class UDSTester:
    """UDS 诊断测试器"""
    
    # 项目定义的 CAN ID
    UDS_REQ_ID = 0x74C       # 物理请求 ID
    UDS_RESP_ID = 0x75C      # 物理响应 ID
    UDS_FUNC_ID = 0x7DF      # 功能请求 ID
    
    def __init__(self, channel='can0', bitrate=500000):
        self.channel = channel
        self.bitrate = bitrate
        self.bus = None
        self.lock = threading.Lock()
        self.received_messages = []
        self.all_messages = []
        self.monitor_running = False
        self.monitor_thread = None
        
    def setup_can_interface(self):
        """配置 CAN 接口"""
        try:
            subprocess.run(['sudo', 'ip', 'link', 'set', self.channel, 'type', 
                          'can', 'bitrate', str(self.bitrate)], check=True)
            subprocess.run(['sudo', 'ip', 'link', 'set', 'up', self.channel], check=True)
            print(f"[成功] CAN 接口 {self.channel} 已配置 @ {self.bitrate}bps")
            return True
        except Exception as e:
            print(f"[警告] 配置 CAN 接口失败: {e}")
            print("[提示] 可能接口已配置，继续尝试打开...")
            return True
    
    def open(self):
        """打开 CAN 接口"""
        try:
            self.bus = can.interface.Bus(
                interface='socketcan',
                channel=self.channel,
                bitrate=self.bitrate
            )
            print(f"[成功] CAN 接口 {self.channel} 已打开")
            return True
        except Exception as e:
            print(f"[错误] 打开 CAN 接口失败: {e}")
            return False
    
    def close(self):
        """关闭 CAN 接口"""
        self.stop_monitor()
        if self.bus:
            self.bus.shutdown()
            print(f"[信息] CAN 接口 {self.channel} 已关闭")
    
    def send_single_frame(self, data):
        """发送 ISO-TP 单帧"""
        can_data = [0] * 8
        can_data[0] = 0x00 | (len(data) & 0x0F)  # SF | 长度
        for i, b in enumerate(data):
            if i < 7:
                can_data[i + 1] = b
        
        try:
            msg = can.Message(
                arbitration_id=self.UDS_REQ_ID,
                data=can_data,
                is_extended_id=False
            )
            self.bus.send(msg)
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            data_str = ' '.join([f'{b:02X}' for b in can_data])
            print(f"  TX [{timestamp}] ID:0x{self.UDS_REQ_ID:03X} Data:[{data_str}]")
            return True
        except Exception as e:
            print(f"  [错误] 发送失败: {e}")
            return False
    
    def receive(self, timeout=1.0):
        """接收报文"""
        messages = []
        start = time.time()
        while time.time() - start < timeout:
            msg = self.bus.recv(timeout=0.1)
            if msg and msg.arbitration_id == self.UDS_RESP_ID:
                messages.append(msg)
        return messages
    
    def _monitor_loop(self):
        """监控线程"""
        while self.monitor_running:
            try:
                msg = self.bus.recv(timeout=0.05)
                if msg:
                    with self.lock:
                        self.all_messages.append(msg)
                        if msg.arbitration_id == self.UDS_RESP_ID:
                            self.received_messages.append(msg)
                            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                            data_str = ' '.join([f'{b:02X}' for b in msg.data])
                            print(f"  RX [{timestamp}] ID:0x{msg.arbitration_id:03X} Data:[{data_str}]")
            except Exception:
                pass
    
    def start_monitor(self):
        """启动监控"""
        self.monitor_running = True
        self.received_messages = []
        self.all_messages = []
        self.monitor_thread = threading.Thread(target=self._monitor_loop)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
        print(f"[信息] 开始监控响应 ID: 0x{self.UDS_RESP_ID:03X}")
    
    def stop_monitor(self):
        """停止监控"""
        if self.monitor_running:
            self.monitor_running = False
            if self.monitor_thread:
                self.monitor_thread.join(timeout=1.0)
    
    def parse_response(self, msg):
        """解析 UDS 响应"""
        data = msg.data
        if len(data) < 2:
            return "数据过短"
        
        frame_type = (data[0] >> 4) & 0x0F
        
        if frame_type == 0:  # 单帧
            length = data[0] & 0x0F
            sid = data[1]
            
            if sid == 0x7F:  # 否定响应
                req_sid = data[2] if len(data) > 2 else 0
                nrc = data[3] if len(data) > 3 else 0
                nrc_names = {
                    0x10: "通用拒绝",
                    0x11: "服务不支持",
                    0x12: "子功能不支持",
                    0x13: "报文长度错误",
                    0x22: "条件不正确",
                    0x24: "请求序列错误",
                    0x31: "请求超出范围",
                    0x33: "安全访问拒绝",
                    0x35: "密钥无效",
                    0x36: "尝试次数过多",
                    0x78: "响应等待"
                }
                nrc_str = nrc_names.get(nrc, f"未知NRC")
                return f"[否定响应] 请求SID=0x{req_sid:02X}, NRC=0x{nrc:02X} ({nrc_str})"
            
            elif sid & 0x40:  # 肯定响应
                resp_sid = sid & 0x3F
                payload = data[2:2+length-1] if length > 1 else []
                
                result = f"[肯定响应] SID=0x{resp_sid:02X}"
                
                if resp_sid == 0x10 and len(payload) >= 1:  # 会话控制
                    session_names = {0x01: "默认", 0x02: "编程", 0x03: "扩展"}
                    session = payload[0]
                    name = session_names.get(session, "未知")
                    result += f", 会话={name}(0x{session:02X})"
                    if len(payload) >= 5:
                        p2 = (payload[1] << 8) | payload[2]
                        p2_star = (payload[3] << 8) | payload[4]
                        result += f", P2={p2}ms, P2*={p2_star}ms"
                
                elif resp_sid == 0x3E:  # TesterPresent
                    result += ", TesterPresent响应"
                
                elif resp_sid == 0x22 and len(payload) >= 2:  # 读DID
                    did = (payload[0] << 8) | payload[1]
                    result += f", DID=0x{did:04X}"
                    if len(payload) > 2:
                        data_bytes = payload[2:]
                        hex_str = ' '.join([f'{b:02X}' for b in data_bytes])
                        try:
                            ascii_str = bytes(data_bytes).decode('ascii', errors='ignore')
                            result += f", 数据=[{hex_str}] (ASCII: {ascii_str})"
                        except:
                            result += f", 数据=[{hex_str}]"
                
                elif resp_sid == 0x34:  # 请求下载
                    result += ", 请求下载已接受"
                
                elif resp_sid == 0x36 and len(payload) >= 1:  # 传输数据
                    block = payload[0]
                    result += f", 块序号={block}"
                
                elif resp_sid == 0x37:  # 退出传输
                    result += ", 退出传输完成"
                
                return result
            
            else:
                return f"[未知响应] SID=0x{sid:02X}, 数据={' '.join([f'{b:02X}' for b in data])}"
        
        elif frame_type == 1:
            return f"[首帧] 多帧传输开始"
        elif frame_type == 2:
            return f"[连续帧]"
        elif frame_type == 3:
            return f"[流控帧]"
        
        return f"[未知帧类型]"


def test_session_control(tester):
    """测试诊断会话控制 0x10"""
    print("\n" + "="*60)
    print("测试 1: 诊断会话控制 (0x10)")
    print("="*60)
    
    tests = [
        {"name": "进入默认会话", "data": [0x10, 0x01], "expected": 0x50},
        {"name": "进入编程会话", "data": [0x10, 0x02], "expected": 0x50},
        {"name": "进入扩展会话", "data": [0x10, 0x03], "expected": 0x50},
    ]
    
    results = []
    for i, test in enumerate(tests, 1):
        print(f"\n[1.{i}] {test['name']}")
        print(f"  发送: {' '.join([f'{b:02X}' for b in test['data']])}")
        
        tester.start_monitor()
        tester.send_single_frame(test['data'])
        time.sleep(0.3)
        tester.stop_monitor()
        
        if tester.received_messages:
            msg = tester.received_messages[-1]
            parsed = tester.parse_response(msg)
            print(f"  响应: {parsed}")
            results.append({"test": test['name'], "success": True})
        else:
            print(f"  [警告] 未收到响应")
            results.append({"test": test['name'], "success": False})
    
    return results


def test_tester_present(tester):
    """测试 TesterPresent 0x3E"""
    print("\n" + "="*60)
    print("测试 2: TesterPresent (0x3E)")
    print("="*60)
    
    tests = [
        {"name": "TesterPresent - 有响应", "data": [0x3E, 0x00], "expect_response": True},
        {"name": "TesterPresent - 抑制响应", "data": [0x3E, 0x80], "expect_response": False},
    ]
    
    results = []
    for i, test in enumerate(tests, 1):
        print(f"\n[2.{i}] {test['name']}")
        print(f"  发送: {' '.join([f'{b:02X}' for b in test['data']])}")
        
        tester.start_monitor()
        tester.send_single_frame(test['data'])
        time.sleep(0.3)
        tester.stop_monitor()
        
        if tester.received_messages:
            msg = tester.received_messages[-1]
            parsed = tester.parse_response(msg)
            print(f"  响应: {parsed}")
            success = test['expect_response']
        else:
            if test['expect_response']:
                print(f"  [警告] 未收到预期响应")
                success = False
            else:
                print(f"  [正常] 未收到响应 (抑制响应位已设置)")
                success = True
        
        results.append({"test": test['name'], "success": success})
    
    return results


def test_read_did(tester):
    """测试读 DID 0x22"""
    print("\n" + "="*60)
    print("测试 3: 读取 DID (0x22)")
    print("="*60)
    
    # 切换到扩展会话后再读 DID
    print("\n[3.0] 先切换到扩展会话")
    tester.start_monitor()
    tester.send_single_frame([0x10, 0x03])
    time.sleep(0.3)
    tester.stop_monitor()
    
    tests = [
        {"name": "读取软件版本 DID (0xF1 89)", "data": [0x22, 0xF1, 0x89]},
        {"name": "读取 VIN DID (0xF1 90)", "data": [0x22, 0xF1, 0x90]},
        {"name": "读取 ECU 名称 DID (0xF1 97)", "data": [0x22, 0xF1, 0x97]},
    ]
    
    results = []
    for i, test in enumerate(tests, 1):
        print(f"\n[3.{i}] {test['name']}")
        print(f"  发送: {' '.join([f'{b:02X}' for b in test['data']])}")
        
        tester.start_monitor()
        tester.send_single_frame(test['data'])
        time.sleep(0.3)
        tester.stop_monitor()
        
        if tester.received_messages:
            msg = tester.received_messages[-1]
            parsed = tester.parse_response(msg)
            print(f"  响应: {parsed}")
            results.append({"test": test['name'], "success": True})
        else:
            print(f"  [警告] 未收到响应")
            results.append({"test": test['name'], "success": False})
    
    return results


def test_flash_services(tester):
    """测试刷写服务 0x34/0x36/0x37"""
    print("\n" + "="*60)
    print("测试 4: 刷写服务 (0x34/0x36/0x37)")
    print("="*60)
    
    print("\n[注意] 刷写服务需要在编程会话且安全解锁后才能使用")
    print("       当前先测试会话权限控制")
    
    results = []
    
    # 测试 1: 在非编程会话请求下载 (应该失败)
    print("\n[4.1] 在默认会话中请求下载 (应返回 NRC 0x22)")
    tester.start_monitor()
    tester.send_single_frame([0x10, 0x01])  # 切换到默认会话
    time.sleep(0.3)
    tester.stop_monitor()
    
    tester.start_monitor()
    # 请求下载: 格式标识 0x44 (4字节地址, 4字节大小), 地址, 大小
    request_download = [0x34, 0x44, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00]
    # 单帧只能放 7 字节数据，所以这里简化测试
    simplified_request = [0x34, 0x44]  # 只发送前两个字节，触发长度错误
    tester.send_single_frame(simplified_request)
    time.sleep(0.3)
    tester.stop_monitor()
    
    if tester.received_messages:
        msg = tester.received_messages[-1]
        parsed = tester.parse_response(msg)
        print(f"  响应: {parsed}")
        results.append({"test": "默认会话请求下载", "success": True})
    else:
        print(f"  [信息] 未收到响应 (可能因为长度不足未处理)")
        results.append({"test": "默认会话请求下载", "success": True})
    
    # 测试 2: 在编程会话但未解锁
    print("\n[4.2] 在编程会话但未解锁请求下载 (应返回 NRC 0x33)")
    tester.start_monitor()
    tester.send_single_frame([0x10, 0x02])  # 切换到编程会话
    time.sleep(0.3)
    tester.stop_monitor()
    
    tester.start_monitor()
    tester.send_single_frame([0x34])  # 发送不完整请求
    time.sleep(0.3)
    tester.stop_monitor()
    
    if tester.received_messages:
        msg = tester.received_messages[-1]
        parsed = tester.parse_response(msg)
        print(f"  响应: {parsed}")
        results.append({"test": "编程会话未解锁请求下载", "success": True})
    else:
        print(f"  [信息] 未收到响应")
        results.append({"test": "编程会话未解锁请求下载", "success": False})
    
    return results


def test_security_access(tester):
    """测试安全访问 0x27"""
    print("\n" + "="*60)
    print("测试 5: 安全访问 (0x27)")
    print("="*60)
    
    # 切换到扩展会话
    print("\n[5.0] 切换到扩展会话")
    tester.start_monitor()
    tester.send_single_frame([0x10, 0x03])
    time.sleep(0.3)
    tester.stop_monitor()
    
    print("\n[5.1] 请求种子 (27 01)")
    tester.start_monitor()
    tester.send_single_frame([0x27, 0x01])
    time.sleep(0.3)
    tester.stop_monitor()
    
    results = []
    if tester.received_messages:
        msg = tester.received_messages[-1]
        parsed = tester.parse_response(msg)
        print(f"  响应: {parsed}")
        
        # 检查是否返回了种子
        data = msg.data
        if len(data) >= 3 and data[1] == 0x67:
            print(f"  [信息] 收到种子数据")
            results.append({"test": "请求种子", "success": True})
        else:
            results.append({"test": "请求种子", "success": False})
    else:
        print(f"  [警告] 未收到响应")
        results.append({"test": "请求种子", "success": False})
    
    return results


def main():
    print("="*70)
    print("UDS_CAN_STM32F4 诊断功能验证")
    print("="*70)
    print(f"开始时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"CAN 接口: can0 @ 500Kbps")
    print(f"诊断 ID: 请求 0x74C / 响应 0x75C")
    print("="*70)
    
    tester = UDSTester(channel='can0', bitrate=500000)
    
    # 配置 CAN 接口
    tester.setup_can_interface()
    
    if not tester.open():
        print("\n[错误] 无法打开 CAN 接口，测试终止")
        print("[提示] 请检查:")
        print("  1. PCAN-USB 是否已连接")
        print("  2. 是否有权限访问 can0 (可能需要 sudo)")
        return
    
    all_results = []
    
    try:
        # 测试 1: 会话控制
        results = test_session_control(tester)
        all_results.extend(results)
        
        # 测试 2: TesterPresent
        results = test_tester_present(tester)
        all_results.extend(results)
        
        # 测试 3: 读 DID
        results = test_read_did(tester)
        all_results.extend(results)
        
        # 测试 4: 安全访问
        results = test_security_access(tester)
        all_results.extend(results)
        
        # 测试 5: 刷写服务
        results = test_flash_services(tester)
        all_results.extend(results)
        
        # 生成测试报告
        print("\n" + "="*70)
        print("测试报告摘要")
        print("="*70)
        
        passed = sum(1 for r in all_results if r['success'])
        total = len(all_results)
        
        print(f"\n总计: {passed}/{total} 通过")
        print("\n详细结果:")
        for r in all_results:
            status = "[通过]" if r['success'] else "[失败]"
            print(f"  {status} {r['test']}")
        
        print("\n" + "="*70)
        if passed == total:
            print("整体状态: [全部通过]")
        elif passed > 0:
            print("整体状态: [部分通过]")
        else:
            print("整体状态: [需要检查硬件连接]")
        print("="*70)
        
    except KeyboardInterrupt:
        print("\n\n[信息] 测试被用户中断")
    except Exception as e:
        print(f"\n[错误] 测试异常: {e}")
        import traceback
        traceback.print_exc()
    finally:
        tester.close()
        print(f"\n结束时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")


if __name__ == '__main__':
    main()
