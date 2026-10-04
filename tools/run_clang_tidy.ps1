#
# Runs clang-tidy over the projects sources using the compile database of a CMake build directory.
#
# Examples:
#   tools/run_clang_tidy.ps1
#   tools/run_clang_tidy.ps1 -Path bibstd/util
#   tools/run_clang_tidy.ps1 -Fix
#
[CmdletBinding()]
param(
  # Directory of the CMake build holding compile_commands.json.
  [string] $BuildDir = "build",
  # Source directories that shall be analyzed, relative to the repository root.
  [string[]] $Path = @("bibstd", "bibstd_test", "bibqml", "verselens"),
  # Apply the fixes clang-tidy suggests instead of only reporting them.
  [switch] $Fix,
  # Amount of parallel clang-tidy processes.
  [int] $Jobs = [Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$compileDb = Join-Path $repoRoot $BuildDir "compile_commands.json"
if(-not (Test-Path $compileDb))
{
  throw "No compile database at $compileDb. Configure the build first, e.g. 'cmake -S . -B $BuildDir'."
}

$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if(-not $clangTidy)
{
  throw "clang-tidy not found on PATH. Install it, e.g. 'pacman -S mingw-w64-ucrt-x86_64-clang-tools-extra'."
}

#
# The compile database also covers the external libraries, so restrict the run to the given source paths.
#
$searchRoots = $Path | ForEach-Object { Join-Path $repoRoot $_ } | Where-Object { Test-Path $_ }
$files = $searchRoots |
  ForEach-Object { Get-ChildItem -Path $_ -Recurse -File -Include *.cpp } |
  ForEach-Object { $_.FullName }

if($files.Count -eq 0)
{
  Write-Host "No sources found in: $($Path -join ', ')"
  exit 0
}

Write-Host "Running clang-tidy on $($files.Count) files with $Jobs jobs ..."

$arguments = @("-p", (Join-Path $repoRoot $BuildDir), "--quiet")
if($Fix)
{
  $arguments += @("--fix", "--fix-errors")
}

$failed = 0
$files | ForEach-Object -ThrottleLimit $Jobs -Parallel {
  $output = & clang-tidy @using:arguments $_ 2>&1
  if($LASTEXITCODE -ne 0 -or $output)
  {
    Write-Output "===== $_"
    $output | Write-Output
  }
} | Tee-Object -Variable report | Out-Host

$failed = ($report | Where-Object { $_ -match "warning:|error:" }).Count
Write-Host "clang-tidy reported $failed diagnostics."
