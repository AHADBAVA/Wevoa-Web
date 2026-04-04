param(
    [string]$BinaryPath = "dist/windows/wevoa.exe",
    [string]$InstallDir = "$env:USERPROFILE\bin",
    [switch]$SkipPathUpdate
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$resolvedBinaryPath = if ([System.IO.Path]::IsPathRooted($BinaryPath)) {
    [System.IO.Path]::GetFullPath($BinaryPath)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BinaryPath))
}
$resolvedInstallDir = if ([System.IO.Path]::IsPathRooted($InstallDir)) {
    [System.IO.Path]::GetFullPath($InstallDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $InstallDir))
}
$destinationPath = Join-Path $resolvedInstallDir "wevoa.exe"
$templatesSource = Join-Path (Split-Path -Parent $resolvedBinaryPath) "templates"
$templatesDestination = Join-Path $resolvedInstallDir "templates"

if (-not (Test-Path $resolvedBinaryPath)) {
    throw "Binary not found: $resolvedBinaryPath. Run scripts/build-release.ps1 first."
}

New-Item -ItemType Directory -Force -Path $resolvedInstallDir | Out-Null
Copy-Item -Force $resolvedBinaryPath $destinationPath
if (Test-Path $templatesDestination) {
    Remove-Item -Recurse -Force $templatesDestination
}
if (Test-Path $templatesSource) {
    Copy-Item -Recurse -Force $templatesSource $templatesDestination
}

if (-not $SkipPathUpdate) {
    $currentUserPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $segments = @()
    if (-not [string]::IsNullOrWhiteSpace($currentUserPath)) {
        $segments = $currentUserPath -split ';'
    }

    if ($segments -notcontains $resolvedInstallDir) {
        $updatedPath = if ([string]::IsNullOrWhiteSpace($currentUserPath)) {
            $resolvedInstallDir
        } else {
            "$currentUserPath;$resolvedInstallDir"
        }

        [Environment]::SetEnvironmentVariable("Path", $updatedPath, "User")
        Write-Host "[Wevoa] Added to user PATH: $resolvedInstallDir"
        Write-Host "[Wevoa] Open a new terminal to use the updated PATH."
    } else {
        Write-Host "[Wevoa] Install directory is already on PATH."
    }
}

Write-Host "[Wevoa] Installed runtime to: $destinationPath"
if (Test-Path $templatesDestination) {
    Write-Host "[Wevoa] Installed starter templates to: $templatesDestination"
}
Write-Host "[Wevoa] Try: wevoa --version"
