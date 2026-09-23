"""打包自包含工程和验证证据，排除编译缓存与解锁检查产物。"""
import hashlib
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def include(path):
    """明确选择交付文件，保留默认锁定固件，不收录临时检查工程。"""
    relative = path.relative_to(ROOT)
    if "__pycache__" in relative.parts or path.suffix == ".exe":
        return False
    if relative.parts[0] in {"Start", "Library", "System", "Hardware", "User", "docs", "tools"}:
        return True
    if relative.parts[0] == "Tests":
        return "Build" not in relative.parts
    if relative.parts[0] == "Objects":
        return path.suffix in {".hex", ".axf"} or path.name == "build.log"
    if relative.parts[0] == "Listings":
        return path.suffix == ".map"
    return relative.as_posix() in {"README.md", "Project.uvprojx", "PROGRESS.md", ".gitignore"}


def main():
    """先写 ZIP，再复核 CRC、完整性和关键入口，旁附 SHA-256。"""
    files = sorted(path for path in ROOT.rglob("*") if path.is_file() and include(path))
    output = ROOT.parent / f"{ROOT.name}.zip"
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in files:
            archive.write(path, f"{ROOT.name}/{path.relative_to(ROOT).as_posix()}")
    with zipfile.ZipFile(output) as archive:
        if archive.testzip() is not None:
            raise ValueError("ZIP CRC 校验失败")
        for path in files:
            name = f"{ROOT.name}/{path.relative_to(ROOT).as_posix()}"
            if archive.read(name) != path.read_bytes():
                raise ValueError(f"归档与源文件不一致：{name}")
        for name in ("Project.uvprojx", "README.md", "docs/validation.json",
                     "Objects/Worm7_Locked/Worm7_Locked.hex", "Objects/Worm11_Locked/Worm11_Locked.hex"):
            archive.getinfo(f"{ROOT.name}/{name}")
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(".zip.sha256").write_text(f"{digest}  {output.name}\n", encoding="ascii")
    print(f"归档校验通过：{len(files)} 个文件，{output.stat().st_size} 字节")
    print(f"SHA-256：{digest}")


if __name__ == "__main__":
    main()
