[CmdletBinding()]
param(
    [string] $ProjectFilePath,
    [string] $UnrealRoot
)

$UnrealCmdPath = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UnrealCmdExists = Test-Path $UnrealCmdPath
if (-not $UnrealCmdExists)
{
    Write-Output "[ERROR] UnrealEditor-Cmd.exe not found at $UnrealCmdPath"
    exit 1
}

$warningCount = 0
$errorCount = 0
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

# format every line; could also leave this entirely to C++, but I want it also runnable from terminal
& $UnrealCmdPath $ProjectFilePath -run=DataValidation 2>&1 | ForEach-Object {
    $line = $_.ToString()
    if ($line -match 'Error:|Failed|: Error')
    {
        Write-Output "[ERROR] $line"
        $errorCount++
    }
    elseif ($line -match 'Warning:|: Warning')
    {
        Write-Output "[WARNING] $line"
        $warningCount++
    }
    elseif ($line -match 'Display:|: Display')
    {
        Write-Output "[INFO] $line"
    }
    else
    {
        Write-Output $line
    }
}

$stopwatch.Stop()
$elapsed = $stopwatch.Elapsed.ToString('hh\:mm\:ss')
Write-Output ""
Write-Output "========== Validation Summary =========="
Write-Output "  Time:     $elapsed"
Write-Output "  Errors:   $errorCount"
Write-Output "  Warnings: $warningCount"
Write-Output "========================================"