param([string]$KeilPath = 'E:\Keil5\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $KeilPath)) { throw "找不到 Keil：$KeilPath" }
# 独立的编译检查工程不会修改默认头文件，也不生成可烧录 HEX。
[xml]$project = Get-Content -LiteralPath (Join-Path $projectDirectory 'Project.uvprojx') -Raw
$checkProject = Join-Path $projectDirectory 'EnabledCompileCheck.uvprojx'
foreach ($target in $project.Project.Targets.Target) {
    $name = $target.TargetName.Replace('_Locked', '_EnabledCheck')
    $target.TargetName = $name
    $common = $target.TargetOption.TargetCommonOption
    $common.OutputName = $name
    $common.OutputDirectory = ".\Tests\Build\$name\"
    $common.ListingPath = ".\Tests\Build\$name\"
    $common.CreateHexFile = '0'
    $defines = $target.TargetOption.TargetArmAds.Cads.VariousControls.Define
    $target.TargetOption.TargetArmAds.Cads.VariousControls.Define = "$defines,APP_HARDWARE_CONFIRMED=1"
    New-Item -ItemType Directory -Force -Path (Join-Path $projectDirectory "Tests\Build\$name") | Out-Null
}
$project.Save($checkProject)
# 本机旧版 uVision 拒绝带 UTF-8 BOM 的工程，返回 15；改写为无 BOM。
$xmlText = [System.IO.File]::ReadAllText($checkProject)
[System.IO.File]::WriteAllText($checkProject, $xmlText, [System.Text.UTF8Encoding]::new($false))
try {
    foreach ($target in $project.Project.Targets.Target) {
        $name = $target.TargetName
        $logRelative = 'enabled-check.log'
        $logPath = Join-Path $projectDirectory $logRelative
        if (Test-Path -LiteralPath $logPath) { Remove-Item -LiteralPath $logPath }
        $process = Start-Process -FilePath $KeilPath -ArgumentList @(
            '-r', 'EnabledCompileCheck.uvprojx', '-t', $name, '-j0', '-o', $logRelative
        ) -WorkingDirectory $projectDirectory -WindowStyle Hidden -Wait -PassThru
        if (-not (Test-Path -LiteralPath $logPath)) {
            throw "Keil 没有生成检查日志，退出码：$($process.ExitCode)"
        }
        $logText = Get-Content -LiteralPath $logPath -Raw
        Write-Output $logText
        if ($process.ExitCode -ne 0 -or $logText -notmatch '0 Error\(s\), 0 Warning\(s\)') {
            throw "解锁配置编译检查失败：$name"
        }
        Copy-Item -LiteralPath $logPath -Destination (Join-Path $projectDirectory "docs\${name}_build.log")
    }
}
finally {
    # 只删除本脚本刚创建的单个检查工程，不递归删除任何目录。
    Remove-Item -LiteralPath $checkProject -ErrorAction SilentlyContinue
}
