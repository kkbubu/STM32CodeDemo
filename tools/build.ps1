param([string]$KeilPath = 'E:\Keil5\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $KeilPath)) { throw "找不到 Keil：$KeilPath" }
foreach ($target in @('Worm7_Locked', 'Worm11_Locked')) {
    New-Item -ItemType Directory -Force -Path (Join-Path $projectDirectory "Objects\$target"), (Join-Path $projectDirectory "Listings\$target") | Out-Null
    $logPath = Join-Path $projectDirectory "Objects\$target\build.log"
    $process = Start-Process -FilePath $KeilPath -ArgumentList @(
        '-r', 'Project.uvprojx', '-t', $target, '-j0', '-o', "Objects\$target\build.log"
    ) -WorkingDirectory $projectDirectory -WindowStyle Hidden -PassThru -Wait
    $logText = Get-Content -LiteralPath $logPath -Raw
    Write-Output $logText
    if ($process.ExitCode -ne 0 -or $logText -notmatch '0 Error\(s\), 0 Warning\(s\)') {
        throw "构建失败或有警告：$target，退出码 $($process.ExitCode)"
    }
}
