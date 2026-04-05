param(
    [string]$ChecksumFile = "dist/SHA256SUMS.txt",
    [string]$BaseDir = "dist"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$resolvedChecksumFile = if ([System.IO.Path]::IsPathRooted($ChecksumFile)) {
    [System.IO.Path]::GetFullPath($ChecksumFile)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $ChecksumFile))
}
$resolvedBaseDir = if ([System.IO.Path]::IsPathRooted($BaseDir)) {
    [System.IO.Path]::GetFullPath($BaseDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BaseDir))
}

if (-not (Test-Path $resolvedChecksumFile)) {
    throw "Checksum file not found: $resolvedChecksumFile"
}
if (-not (Test-Path $resolvedBaseDir)) {
    throw "Base directory not found: $resolvedBaseDir"
}

$failures = @()
$lines = Get-Content -Path $resolvedChecksumFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
foreach ($line in $lines) {
    if ($line -notmatch '^([a-f0-9]{64})\s{2}(.+)$') {
        $failures += "Malformed checksum line: $line"
        continue
    }

    $expected = $matches[1].ToLowerInvariant()
    $relativePath = $matches[2]
    $targetPath = Join-Path $resolvedBaseDir ($relativePath -replace '/', [System.IO.Path]::DirectorySeparatorChar)

    if (-not (Test-Path $targetPath)) {
        $failures += "Missing file: $relativePath"
        continue
    }

    $actual = (Get-FileHash -Path $targetPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) {
        $failures += "Checksum mismatch: $relativePath"
    }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    throw "Checksum verification failed."
}

Write-Host "[Wevoa] Checksum verification passed for $resolvedChecksumFile"
