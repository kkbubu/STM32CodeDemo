param([string]$TccPath = 'E:\Program Files\MATLAB\R2021a\sys\tcc\win64\tcc.exe')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $TccPath)) { throw "找不到测试编译器：$TccPath" }
$results = @()
foreach ($channels in @(7, 11)) {
    foreach ($mode in @('locked', 'enabled', 'feedback')) {
        $confirmed = [int]($mode -ne 'locked')
        $feedback = [int]($mode -eq 'feedback')
        $executable = Join-Path $projectDirectory "Tests\test_${channels}_${mode}.exe"
        $sources = @('Tests\test_robot.c', 'User\Pneumatic.c', 'User\Robot.c', 'User\Command.c') | ForEach-Object { Join-Path $projectDirectory $_ }
        & $TccPath -I (Join-Path $projectDirectory 'User') "-DAPP_VALVE_COUNT=$channels" "-DAPP_HARDWARE_CONFIRMED=$confirmed" "-DAPP_USE_FEEDBACK=$feedback" @sources -o $executable
        if ($LASTEXITCODE -ne 0) { throw '测试程序编译失败' }
        $result = & $executable
        Write-Output $result
        if ($LASTEXITCODE -ne 0) { throw "测试失败：$channels $mode" }
        $results += $result
    }
}
foreach ($channels in @(7, 11)) {
    foreach ($polarity in @(0, 1)) {
        $executable = Join-Path $projectDirectory "Tests\test_gpio_${channels}_${polarity}.exe"
        $sources = @('Tests\test_board.c', 'Hardware\Board.c', 'User\Pneumatic.c') | ForEach-Object { Join-Path $projectDirectory $_ }
        & $TccPath -I (Join-Path $projectDirectory 'Tests\stubs') -I (Join-Path $projectDirectory 'User') -I (Join-Path $projectDirectory 'Hardware') "-DAPP_VALVE_COUNT=$channels" "-DAPP_MOS_ACTIVE_HIGH=$polarity" @sources -o $executable
        if ($LASTEXITCODE -ne 0) { throw 'GPIO 测试编译失败' }
        $result = & $executable
        Write-Output $result
        if ($LASTEXITCODE -ne 0) { throw "GPIO 测试失败：$channels $polarity" }
        $results += $result
    }
}
$results | Set-Content -LiteralPath (Join-Path $projectDirectory 'docs\test-results.txt') -Encoding utf8
