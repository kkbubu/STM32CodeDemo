"""验证自包含工程、固定依赖、目标构建与 HEX；不代表硬件实测。"""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def check_hex(path):
    """校验 Intel HEX 记录、芯片容量、初始栈和有效 Thumb 复位入口。"""
    memory = {}
    upper_address = 0
    eof_seen = False
    for line in path.read_text(encoding="ascii").splitlines():
        if eof_seen or not line.startswith(":"):
            raise ValueError("HEX 记录起止错误")
        record = bytes.fromhex(line[1:])
        if len(record) < 5 or len(record) != record[0] + 5 or sum(record) % 256:
            raise ValueError("HEX 长度或校验和错误")
        offset, kind = int.from_bytes(record[1:3], "big"), record[3]
        data = record[4:-1]
        if kind == 0:
            for index, value in enumerate(data):
                address = upper_address + offset + index
                if not 0x08000000 <= address < 0x08010000 or address in memory:
                    raise ValueError("HEX 超过 64 KB 或出现重复地址")
                memory[address] = value
        elif kind == 4 and len(data) == 2 and offset == 0:
            upper_address = int.from_bytes(data, "big") << 16
        elif kind == 1 and len(data) == 0 and offset == 0:
            eof_seen = True
        elif kind == 5 and len(data) == 4 and offset == 0:
            pass
        else:
            raise ValueError(f"HEX 不支持的记录类型：{kind}")
    if not eof_seen:
        raise ValueError("HEX 缺少结束记录")
    vector = bytes(memory[address] for address in range(0x08000000, 0x08000008))
    stack, reset = int.from_bytes(vector[:4], "little"), int.from_bytes(vector[4:], "little")
    if not 0x20000000 < stack <= 0x20005000 or stack % 8 or not (reset & 1) or (reset & ~1) not in memory:
        raise ValueError("初始栈或 Thumb 入口错误")
    return dict(data_bytes=len(memory), initial_stack=hex(stack), reset_vector=hex(reset),
                sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def check_build(log_path):
    """读取真实编译日志，校验错误、警告以及 Flash/RAM 大小。"""
    log = log_path.read_text(encoding="utf-8-sig")
    if "0 Error(s), 0 Warning(s)" not in log:
        raise ValueError(f"构建未通过：{log_path}")
    match = re.search(r"Code=(\d+) RO-data=(\d+) RW-data=(\d+) ZI-data=(\d+)", log)
    if not match:
        raise ValueError("缺少链接大小记录")
    code, ro, rw, zi = map(int, match.groups())
    if code + ro + rw > 65536 or rw + zi > 20480:
        raise ValueError("固件超出 STM32F103C8 容量")
    return dict(flash_bytes=code + ro + rw, ram_bytes=rw + zi, errors=0, warnings=0)


def main():
    """保存当前文件和验证结果，生成可复核的 JSON 报告。"""
    config = (ROOT / "User/AppConfig.h").read_text(encoding="utf-8")
    locked = bool(re.search(r"^#define APP_HARDWARE_CONFIRMED 0$", config, re.M))
    tree = ET.parse(ROOT / "Project.uvprojx")
    targets = []
    for target in tree.findall(".//Target"):
        name = target.findtext("TargetName")
        if target.findtext(".//Device") != "STM32F103C8":
            raise ValueError("目标芯片不匹配")
        defines = target.findtext(".//Cads/VariousControls/Define")
        if "USE_HAL_DRIVER" not in defines or "STM32F103xB" not in defines:
            raise ValueError("HAL/芯片宏缺失")
        files = []
        for file in target.findall(".//Groups/Group/Files/File"):
            path = (ROOT / file.findtext("FilePath").replace("\\", "/")).resolve()
            if not path.is_relative_to(ROOT) or not path.is_file():
                raise ValueError(f"存在外部或缺失文件：{path}")
            files.append(path)
        if len(files) != len(set(files)):
            raise ValueError("重复源文件")
        for include in target.findtext(".//Cads/VariousControls/IncludePath").split(";"):
            directory = (ROOT / include.replace("\\", "/")).resolve()
            if not directory.is_relative_to(ROOT) or not directory.is_dir():
                raise ValueError("包含路径不自包含")
        log_path = ROOT / "Objects" / name / "build.log"
        # 修改源码后不允许复用旧构建结果。
        latest_source = max(p.stat().st_mtime for folder in ("User", "System", "Hardware")
                            for p in (ROOT / folder).glob("*.[ch]"))
        if log_path.stat().st_mtime < latest_source:
            raise ValueError("源码晚于构建日志，请重新构建")
        result = dict(target=name, sources=len(files), **check_build(log_path))
        result["hex"] = check_hex(ROOT / "Objects" / name / f"{name}.hex")
        targets.append(result)
    vendor_count = 0
    for vendor in json.loads((ROOT / "docs/vendor-manifest.json").read_text(encoding="utf-8")):
        for relative, sha256 in vendor["files"].items():
            if hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() != sha256:
                raise ValueError(f"厂商文件哈希变化：{relative}")
            vendor_count += 1
    for name in ("wiring", "pneumatics", "gait"):
        ET.parse(ROOT / "docs" / f"{name}.svg")
        if not (ROOT / "docs" / f"{name}.png").is_file():
            raise ValueError("缺少渲染图")
    tests = (ROOT / "docs/test-results.txt").read_text(encoding="utf-8-sig").splitlines()
    if len(tests) != 10 or not all(line.startswith("PASS ") for line in tests):
        raise ValueError("六组状态机与四组 GPIO 验证记录不完整")
    enabled_checks = [dict(target=f"Worm{count}_EnabledCheck",
                           **check_build(ROOT / "docs" / f"Worm{count}_EnabledCheck_build.log"))
                      for count in (7, 11)]
    source_hashes = {}
    for folder in ("User", "Hardware", "System", "tools", "Tests"):
        for path in (ROOT / folder).rglob("*"):
            if path.is_file() and path.suffix in (".h", ".c", ".py", ".ps1"):
                source_hashes[path.relative_to(ROOT).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    report = dict(verified_at_utc=datetime.now(timezone.utc).isoformat(), device="STM32F103C8T6",
                  default_hardware_lock=locked, hardware_tested=False, targets=targets,
                  enabled_compile_checks=enabled_checks, host_test_runs=tests,
                  vendor_files_verified=vendor_count, source_sha256=source_hashes,
                  limitations=["未烧录或运行真实芯片", "未验证机械抓力及垂直爬行",
                               "反馈测试使用构造输入，不等同于传感器验证"])
    (ROOT / "docs/validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in report.items() if k != "source_sha256"}, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
