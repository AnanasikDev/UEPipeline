# Package and cook an Unreal Engine project
# .\stage2_build.ps1 -UnrealRoot "C:\UE_5.4" -ProjectPath "D:\MyGame\MyGame.uproject" -OutputDir "D:\Builds" -Config Shipping

[CmdletBinding()]
param(
    [string] $UnrealRoot,
    [string] $ProjectPath,
    [string] $OutputDir,

    [ValidateSet('Development', 'Shipping')]
    [string] $Config = 'Development',

    [ValidateSet('Win64', 'Linux', 'Mac')]
    [string] $Platform = 'Win64'
)

$uat = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path $uat))
{
    throw ('RunUAT.bat not found at ' + $uat + '. Check -UnrealRoot or run with -Setup')
}

if (-not (Test-Path $ProjectPath))
{
    throw ('Project not found at ' + $ProjectPath + '. Check -ProjectPath or run with -Setup')
}

if (-not (Test-Path $OutputDir))
{
    throw "-OutputDir is required when not running -Setup. Example: .\stage2_build.ps1 -OutputDir 'D:\Builds'"
}

try
{
    $Timestamp = Get-Date -Format 'yyyy_MM_dd-HH_mm_ss'
    $BuildDir  = Join-Path $OutputDir ('build-' + $Config + '-' + $Timestamp)
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    $LogFile = Join-Path $BuildDir 'build.log'

    Write-Host ''
    Write-Host '=== Unreal Engine Build Script ===' -ForegroundColor Cyan
    Write-Host ('Project  : ' + $ProjectPath)
    Write-Host ('Engine   : ' + $UnrealRoot)
    Write-Host ('Config   : ' + $Config)
    Write-Host ('Platform : ' + $Platform)
    Write-Host ('Output   : ' + $BuildDir)
    Write-Host ('Log      : ' + $LogFile)
    Write-Host ''

    Write-Host '=== Packaging and Cooking (this takes a while) ===' -ForegroundColor Cyan

    $uatArgs = @(
        'BuildCookRun',
        ('-project=' + $ProjectPath),
        '-noP4',
        ('-platform=' + $Platform),
        ('-clientconfig=' + $Config),
        '-cook',
        '-allmaps',
        '-build',
        '-stage',
        '-pak',
        '-archive',
        ('-archivedirectory=' + $BuildDir)
    )

    & $uat @uatArgs 2>&1 | Tee-Object -FilePath $LogFile

    if ($LASTEXITCODE -ne 0)
    {
        throw ('UAT failed with exit code ' + $LASTEXITCODE + '. See ' + $LogFile)
    }

    $StagedDir = Join-Path $BuildDir 'Windows'
    if (-not (Test-Path $StagedDir))
    {
        throw ('Expected staged output at ' + $StagedDir + ' but it does not exist. Check ' + $LogFile)
    }

    Write-Host ''
    Write-Host '=== Done ===' -ForegroundColor Green
    Write-Host ('[HOOK] Build output located at: ' + $StagedDir)
}
catch
{
    $msg = $_.ToString()
    Write-Host ''
    Write-Host ('FAILED: ' + $msg) -ForegroundColor Red
    exit 1
}