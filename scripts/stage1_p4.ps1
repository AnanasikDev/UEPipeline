# p4sync.ps1 - Ensure clean workspace, then sync to latest
#
# Usage:
#   .\p4sync.ps1 -ProjectRoot "D:\Perforce\MyGame"
#   .\p4sync.ps1                                      # uses current directory

[CmdletBinding()]
param(
    [string] $ProjectRoot
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $ProjectRoot)
{
    $ProjectRoot = Get-Location
}
$ProjectRoot = (Resolve-Path $ProjectRoot).Path

# ---------- Run pre-flight ----------

Write-Host ''
Write-Host '=== Pre-Sync Check ===' -ForegroundColor Cyan
Write-Host ''

$checkScript = Join-Path $ScriptDir 'p4check.ps1'
if (-not (Test-Path $checkScript))
{
    Write-Host ('FAILED: p4check.ps1 not found at ' + $checkScript) -ForegroundColor Red
    exit 1
}

$result = & $checkScript -ProjectRoot $ProjectRoot

# ---------- Check for local changes ----------

if ($result.LocalChanges -and $result.LocalChanges.Count -gt 0)
{
    Write-Host ''
    Write-Host '=== Cannot Sync ===' -ForegroundColor Red
    Write-Host ''
    Write-Host ('  You have ' + $result.LocalChanges.Count.ToString() + ' file(s) checked out:') -ForegroundColor Red
    Write-Host ''

    foreach ($file in $result.LocalChanges)
    {
        $display = $file.ToString().Trim()
        if ($display.Length -gt 120)
        {
            $display = $display.Substring(0, 117) + '...'
        }
        Write-Host ('    ' + $display) -ForegroundColor DarkYellow
    }

    Write-Host ''
    Write-Host '  You must resolve these before syncing. Options:' -ForegroundColor Yellow
    Write-Host ''
    Write-Host '    1. Submit your changes:   p4 submit' -ForegroundColor White
    Write-Host '    2. Shelve for later:      p4 shelve -c <CL>  then  p4 revert <files>' -ForegroundColor White
    Write-Host '    3. Revert (discard):      p4 revert <files>' -ForegroundColor White
    Write-Host ''
    Write-Host '  Syncing with open files risks merge conflicts and unreliable builds.' -ForegroundColor DarkGray
    Write-Host ''

    exit 1
}

Write-Host ''
Write-Host '  Workspace is clean. No local changes.' -ForegroundColor Green

# ---------- Check if sync is needed ----------

if ($result.IsSynced)
{
    Write-Host '  Already at latest revision. Nothing to sync.' -ForegroundColor Green
    Write-Host ''

    $output = [ordered]@{
        SyncedCL = $result.SyncedCL
        Action   = 'none'
        Success  = $true
    }
    return $output
}

# ---------- Sync to latest ----------

Write-Host ''
Write-Host '=== Syncing to Latest ===' -ForegroundColor Cyan
Write-Host ''

$p4Scope = $ProjectRoot + '\...'

$syncRaw = & p4 sync $p4Scope 2>&1 | Out-String

if ($LASTEXITCODE -ne 0)
{
    Write-Host ('FAILED: p4 sync returned exit code ' + $LASTEXITCODE.ToString()) -ForegroundColor Red
    Write-Host $syncRaw -ForegroundColor DarkYellow
    exit 1
}

# Get the new CL after sync
$newHaveRaw = & p4 changes -m1 -s submitted ($p4Scope + '#have') 2>&1 | Out-String
$newCL = ''
if ($newHaveRaw -match 'Change\s+(\d+)')
{
    $newCL = $Matches[1]
}

Write-Host ''
Write-Host '=== Sync Complete ===' -ForegroundColor Green
Write-Host ('  Previous CL: ' + $result.SyncedCL)
Write-Host ('  Current CL:  ' + $newCL)
Write-Host ''

$output = [ordered]@{
    SyncedCL   = $newCL
    PreviousCL = $result.SyncedCL
    Action     = 'synced'
    Success    = $true
}
return $output