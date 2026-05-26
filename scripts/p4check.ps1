# p4check.ps1 - Perforce pre-flight checks
#
# Usage:
#   .\p4check.ps1                  # run all checks, warn on issues
#   .\p4check.ps1 -ShowSyncStatus  # also show if you're behind latest

[CmdletBinding()]
param(
    [switch] $ShowSyncStatus,
    [string] $ProjectRoot
)

# ---------- Helpers ----------

function Write-OK    
{ 
    param([string]$Msg)
    Write-Host ('  OK    ' + $Msg) -ForegroundColor Green
}
function Write-Warn
{ 
    param([string]$Msg)
    Write-Host ('  WARNING  ' + $Msg) -ForegroundColor Yellow
}
function Write-Fail
{ 
    param([string]$Msg)
    Write-Host ('  FAIL  ' + $Msg) -ForegroundColor Red
}

$script:FailCount = 0

function Add-Failure {
    param([string]$Msg)
    $script:FailCount++
    Write-Warn $Msg
}

# ---------- Check: p4 is available and connected ----------

Write-Host ''
Write-Host '=== Perforce Pre-Flight ===' -ForegroundColor Cyan
Write-Host ''

# Verify p4 command exists
$p4cmd = Get-Command p4 -ErrorAction SilentlyContinue
if (-not $p4cmd)
{
    Write-Fail 'p4.exe not found on PATH. Install Perforce command-line tools.'
    exit 1
}

# Verify connection
try 
{
    $infoRaw = & p4 info 2>&1
    $infoText = $infoRaw | Out-String

    if ($infoText -match 'Connect to server failed')
    {
        Write-Fail 'Cannot connect to Perforce server. Check P4PORT and network.'
        exit 1
    }
}
catch
{
    Write-Fail ('p4 info failed: ' + $_.ToString())
    exit 1
}

# Parse useful fields from p4 info
$p4User       = ''
$p4Client     = ''
$p4Server     = ''
$p4ClientRoot = ''

foreach ($line in $infoRaw)
{
    $lineStr = $line.ToString()
    if ($lineStr -match '^User name:\s+(.+)$')      { $p4User       = $Matches[1].Trim() }
    if ($lineStr -match '^Client name:\s+(.+)$')    { $p4Client     = $Matches[1].Trim() }
    if ($lineStr -match '^Server address:\s+(.+)$') { $p4Server     = $Matches[1].Trim() }
    if ($lineStr -match '^Client root:\s+(.+)$')    { $p4ClientRoot = $Matches[1].Trim() }
}

Write-Host ('  User     : ' + $p4User)
Write-Host ('  Client   : ' + $p4Client)
Write-Host ('  Server   : ' + $p4Server)
Write-Host ('  Root     : ' + $p4ClientRoot)
Write-Host ''

if (-not $p4Client)
{
    Write-Fail 'No workspace (client) detected. Set P4CLIENT or run from a workspace directory.'
    exit 1
}

Write-OK 'Connected to Perforce'

# ---------- Determine project scope ----------

if (-not $ProjectRoot)
{
    # Default to current directory if not specified
    $ProjectRoot = Get-Location
}

# Resolve to absolute path
$ProjectRoot = (Resolve-Path $ProjectRoot).Path

# Verify it's inside the client root
if (-not $ProjectRoot.StartsWith($p4ClientRoot, [System.StringComparison]::OrdinalIgnoreCase))
{
    Write-Fail ('Project root ' + $ProjectRoot + ' is not inside client root ' + $p4ClientRoot)
    exit 1
}

# This is the path we pass to every p4 command
# Trailing \... means "everything under this folder recursively"
$p4Scope = $ProjectRoot + '\...'

Write-Host ('  Scope  : ' + $p4Scope)
Write-Host ''

# ---------- Check: currently checked-out files (pending changes) ----------

Write-Host ''
Write-Host '--- Checked-out files ---' -ForegroundColor Cyan

$localChanges = $true

$openedRaw = & p4 opened $p4Scope 2>&1 | Out-String

if ($openedRaw -match 'not opened')
{
    Write-OK 'No files checked out. Workspace is clean.'
    $localChanges = $false
}
elseif ([string]::IsNullOrWhiteSpace($openedRaw))
{
    Write-OK 'No files checked out. Workspace is clean.'
    $localChanges = $false
}
else
{
    $openedLines = ($openedRaw.Trim() -split "`n") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $openedCount = $openedLines.Count

    Add-Failure ($openedCount.ToString() + ' file(s) currently checked out:')

    foreach ($line in $openedLines)
    {
        $trimmed = $line.ToString().Trim()
        # Show just the depot path and action, truncated for readability
        if ($trimmed.Length -gt 120)
        {
            $trimmed = $trimmed.Substring(0, 117) + '...'
        }
        Write-Host ('         ' + $trimmed) -ForegroundColor DarkYellow
    }

    Write-Host ''
    Write-Host '         These files are NOT in the depot yet.' -ForegroundColor DarkYellow
    Write-Host '         A build from this machine will include local changes others do not have.' -ForegroundColor DarkYellow
}

# ---------- Check: latest submitted changelist on server ----------

Write-Host ''
Write-Host '--- Latest server changelist ---' -ForegroundColor Cyan

$latestRaw = & p4 changes -m1 -s submitted '//...' 2>&1 | Out-String
$latestCL  = ''

if ($latestRaw -match 'Change\s+(\d+)') 
{
    $latestCL = $Matches[1]
    Write-OK ('Latest server CL:  ' + $latestCL)
}
else
{
    Add-Failure 'Could not determine latest server changelist.'
}

# ---------- Check: current synced changelist ----------

Write-Host ''
Write-Host '--- Synced changelist ---' -ForegroundColor Cyan

# The latest CL you have synced to
$havePath = $p4Scope + '#have'
$haveRaw = & p4 changes -m1 -s submitted $havePath 2>&1 | Out-String
$haveCL  = ''

if ($haveRaw -match 'Change\s+(\d+)')
{
    $haveCL = $Matches[1]
    Write-OK ('Latest synced CL: ' + $haveCL)
}
else
{
    Add-Failure 'Could not determine synced changelist.'
}

$targetFolder = $p4Scope

Write-Host "======================================" -ForegroundColor Cyan
Write-Host " Checking Perforce Status for Folder: " -ForegroundColor Cyan
Write-Host " $targetFolder" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan

# check for Opened Files
Write-Host "`n[1] Checking for opened files..." -ForegroundColor White
$openedFiles = p4 opened $targetFolder 2>&1 | Out-String

# Check if the output contains the standard Perforce "not opened" message
if ($openedFiles -match "not opened on this client"  -or [string]::IsNullOrWhiteSpace($openedFiles))
{
    Write-Host "[OK] No opened files found." -ForegroundColor Green
}
else
{
    Write-Host "[ERROR] Opened files found:" -ForegroundColor Yellow
    $openedFiles | ForEach-Object { Write-Host "    $_" }
}

# 2. Check for Unsynced Files
Write-Host "`n[2] Checking for unsynced files..." -ForegroundColor White
# The -n flag previews the sync without actually downloading anything
$unsyncedFiles = p4 sync -n $targetFolder 2>&1 | Out-String

# Check if the output says files are already up-to-date
if ($unsyncedFiles -match "up-to-date" -or [string]::IsNullOrWhiteSpace($unsyncedFiles))
{
    Write-Host "[OK] All files are fully synced (up-to-date)." -ForegroundColor Green
}
else
{
    Write-Host "[WARNING] Unsynced files found (you are behind the head revision):" -ForegroundColor Yellow
    $unsyncedFiles | ForEach-Object { Write-Host "    $_" }
}

Write-Host "`nDone." -ForegroundColor Cyan

# ---------- Check: pending changelists (not just default) ----------

Write-Host ''
Write-Host '--- Pending changelists ---' -ForegroundColor Cyan

$pendingRaw = & p4 changes -s pending -c $p4Client 2>&1 | Out-String

if ([string]::IsNullOrWhiteSpace($pendingRaw) -or ($pendingRaw -match 'no such'))
{
    Write-OK 'No pending changelists.'
}
else
{
    $pendingLines = ($pendingRaw.Trim() -split "`n") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $pendingCount = $pendingLines.Count

    Write-Host ('  INFO  ' + $pendingCount.ToString() + ' pending changelist(s):') -ForegroundColor DarkCyan
    foreach ($line in $pendingLines)
    {
        Write-Host ('         ' + $line.ToString().Trim()) -ForegroundColor DarkCyan
    }
}

# ---------- Summary ----------

Write-Host ''
Write-Host '=== Summary ===' -ForegroundColor Cyan

if ($script:FailCount -eq 0)
{
    Write-Host ('[SUCCESS] All checks passed. Safe to build.') -ForegroundColor Green
    Write-Host ('  Synced CL: ' + $haveCL)
}
else
{
    $label = 'WARNING'
    Write-Host ('  ' + $label + ': ' + $script:FailCount.ToString() + ' issue(s) found.') -ForegroundColor Yellow
}

Write-Host ''

$result = [ordered]@{
    User       = $p4User
    Client     = $p4Client
    Server     = $p4Server
    SyncedCL   = $haveCL
    LatestCL   = $latestCL
    IsSynced   = ($haveCL -eq $latestCL)
    LocalChanges = $localChanges
    FailCount  = $script:FailCount
    ClientRoot = $p4ClientRoot
}

return $result