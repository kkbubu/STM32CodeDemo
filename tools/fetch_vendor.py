"""下载固定版本的官方依赖，只提取工程需要的文件并记录哈希。"""
import hashlib
import io
import json
from pathlib import Path
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = [
    ("STMicroelectronics/stm32f1xx_hal_driver", "77fbb30b7a1d02533980400083e48c559aae5a4f", "Library/HAL", ("Inc/", "Src/", "LICENSE")),
    ("STMicroelectronics/cmsis_device_f1", "8a76309ed1250d817e9c888c4417171d2ba3ba63", "Start/Device", ("Include/", "Source/Templates/", "LICENSE")),
    ("ARM-software/CMSIS_5", "5.9.0", "Start/CMSIS", ("CMSIS/Core/Include/", "LICENSE.txt")),
]


def main():
    """保留厂商原文；版本与 CubeF1 v1.8.6 对应组件一致。"""
    manifest = []
    for repo, revision, destination, prefixes in PACKAGES:
        url = f"https://codeload.github.com/{repo}/zip/{revision}"
        data = urllib.request.urlopen(url, timeout=120).read()
        archive = zipfile.ZipFile(io.BytesIO(data))
        files = {}
        for entry in archive.infolist():
            relative = entry.filename.partition("/")[2]
            if entry.is_dir() or not relative.startswith(prefixes):
                continue
            if repo.endswith("CMSIS_5"):
                relative = relative.removeprefix("CMSIS/Core/")
            target = ROOT / destination / relative
            if not target.resolve().is_relative_to(ROOT.resolve()):
                raise ValueError("依赖包含非法路径")
            target.parent.mkdir(parents=True, exist_ok=True)
            contents = archive.read(entry)
            target.write_bytes(contents)
            files[target.relative_to(ROOT).as_posix()] = hashlib.sha256(contents).hexdigest()
        manifest.append(dict(repo=repo, revision=revision, url=url,
                             archive_sha256=hashlib.sha256(data).hexdigest(), files=files))
        print(f"已获取 {repo}@{revision}，{len(files)} 个文件", flush=True)
    (ROOT / "docs/vendor-manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
