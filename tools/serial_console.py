"""串口控制台：自动每秒发送心跳；只发送用户输入的动作，不自动 ARM。

用法：python -X utf8 tools/serial_console.py COM3
依赖：python -m pip install pyserial
"""
import argparse
import queue
import threading
import time


def read_commands(commands):
    """将终端输入传给串口线程，避免用户等待输入时心跳中断。"""
    while True:
        try:
            command = input().strip()
        except EOFError:
            commands.put("QUIT")
            return
        commands.put(command)
        if command == "QUIT":
            return


def main():
    """串口断连不重连、不重放指令；退出仅发 STOP，绝不自动泄压。"""
    parser = argparse.ArgumentParser(description="蠕虫机器人串口控制台，115200 8N1")
    parser.add_argument("port", help="例如 COM3；连接前先托住机器人")
    args = parser.parse_args()
    try:
        import serial
    except ImportError as error:
        raise SystemExit("缺少 pyserial，请执行 python -m pip install pyserial") from error
    commands = queue.Queue()
    try:
        with serial.Serial(args.port, 115200, timeout=0.05, write_timeout=0.5) as port:
            print("已连接。自动每秒 PING；不会自动启动。STATUS 查询；QUIT 退出。")
            print("ARM / RUN FWD 1 / RUN REV 1 / GRIP FRONT IN / GRIP FRONT OUT / STOP")
            print("RELEASE 会全部泄压，仅在已托住且 PA8 许可断开后使用。")
            threading.Thread(target=read_commands, args=(commands,), daemon=True).start()
            next_ping = 0.0
            pending = bytearray()
            port.write(b"STATUS\r\n")
            try:
                while True:
                    now = time.monotonic()
                    if now >= next_ping:
                        port.write(b"PING\r\n")
                        next_ping = now + 1.0
                    try:
                        command = commands.get_nowait()
                    except queue.Empty:
                        command = ""
                    if command == "QUIT":
                        break
                    if command:
                        try:
                            payload = command.encode("ascii")
                        except UnicodeEncodeError:
                            print("命令必须为 ASCII 大写英文。")
                            continue
                        if len(payload) > 63:
                            print("命令过长，未发送。")
                            continue
                        port.write(payload + b"\r\n")
                    pending.extend(port.read(256))
                    while b"\n" in pending:
                        line, _, remainder = pending.partition(b"\n")
                        pending = bytearray(remainder)
                        print(line.decode("ascii", errors="replace").rstrip())
                    if len(pending) > 4096:
                        raise RuntimeError("接收数据未正常分行，停止会话")
            finally:
                # 连接仍在时尝试冻结动作；断连时板端心跳保护接管。
                port.write(b"STOP\r\n")
    except KeyboardInterrupt:
        print("已退出；已尝试发送 STOP，请确认设备状态。")
    except (serial.SerialException, OSError, RuntimeError) as error:
        raise SystemExit(f"串口异常：{error}。检查设备状态；未发送 RELEASE。") from error


if __name__ == "__main__":
    main()
