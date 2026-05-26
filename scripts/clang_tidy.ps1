# In powershell: "& D:\EpicGames\UE_5.7\Engine\Build\BatchFiles\Build.bat" HellowEditor Win64 Development -project="C:\Archive\Code\Unreal Engine\Hellow\Hellow.uproject" -mode=GenerateClangDatabase
# In powershell:  & ".\clang_tidy.ps1" -SourceDir "C:\Archive\Code\Unreal Engine\Hellow\Source" -CompileCommandsJson ""D:\EpicGames\UE_5.7\compile_commands.json""

param(
    [Parameter(Mandatory)][string]$SourceDir,
    [Parameter(Mandatory)][string]$CompileCommandsJson
)

# --- Validate inputs ---
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy)
{ 
    Write-Error "clang-tidy not found in PATH"
    exit 1
}

if (-not (Test-Path $SourceDir))
{
    Write-Error "SourceDir not found: $SourceDir"
    exit 1
}

if (-not (Test-Path $CompileCommandsJson)) 
{
    Write-Error "compile_commands.json not found at: $CompileCommandsJson"
    Write-Host @"

You need to generate it from Unreal Build Tool first. See instructions
in the comments at the top of this script, or run:

  <EngineDir>\Engine\Build\BatchFiles\Build.bat ^
      <YourProject>Editor Win64 Development ^
      -project="<path\to\YourProject.uproject>" ^
      -mode=GenerateClangDatabase

Then pass the path to the resulting compile_commands.json as -CompileCommandsJson.
"@
    exit 1
}

$dbDir = Split-Path $CompileCommandsJson -Parent

# replace backslashes with forward slashes
$normalizedSourceDir = (Resolve-Path $SourceDir).Path.Replace('\', '/')
$escapedSourceDir    = [regex]::Escape($normalizedSourceDir)

# (?i) = case-insensitive (e.g. Windows drive letters: C: vs c:)
# clang uses forward slahses, so we use the escaped (normalized) path
$headerFilter = "(?i)^$escapedSourceDir"

# collect only MY .cpp files
$files = @(Get-ChildItem -Path $SourceDir -Recurse -Include *.cpp)
if ($files.Count -eq 0)
{
    Write-Warning "No .cpp files found in $SourceDir"
    exit 0
}

Write-Host "compile_commands.json : $CompileCommandsJson"
Write-Host "Source dir            : $normalizedSourceDir"
Write-Host "Files to analyze      : $($files.Count)"
Write-Host "Header filter         : $headerFilter"
Write-Host ""

# run clang-tidy on each project file
$failed = 0

foreach ($f in $files) {
    Write-Host "-- $($f.Name)" # report file name being analyzed

    & clang-tidy `
        -p="$dbDir" `
        --config-file="$PSScriptRoot\.clang-tidy" `
        --header-filter="$headerFilter" `
        --quiet `
        $f.FullName

    if ($LASTEXITCODE -ne 0)
    { 
        $failed++
    }
}

Write-Host ""
if ($failed -gt 0)
{
    Write-Error "$failed file(s) had issues"
    exit 1
}

Write-Host "All clean."
