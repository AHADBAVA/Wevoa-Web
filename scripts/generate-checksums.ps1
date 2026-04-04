param(
    [string]$DistDir = "dist",
    [string]$OutputFile = "SHA256SUMS.txt"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$resolvedDistDir = if ([System.IO.Path]::IsPathRooted($DistDir)) {
    [System.IO.Path]::GetFullPath($DistDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $DistDir))
}

if (-not (Test-Path $resolvedDistDir)) {
    throw "Distribution directory not found: $resolvedDistDir"
}

$outputPath = Join-Path $resolvedDistDir $OutputFile
$files = Get-ChildItem -Path $resolvedDistDir -Recurse -File |
    Where-Object { $_.FullName -ne $outputPath }

$distPrefix = $resolvedDistDir.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar

$lines = foreach ($file in $files) {
    $hash = Get-FileHash -Path $file.FullName -Algorithm SHA256
    $relativePath = $file.FullName
    if ($relativePath.StartsWith($distPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $relativePath.Substring($distPrefix.Length)
    }
    $relativePath = $relativePath.Replace('\', '/')
    "{0}  {1}" -f $hash.Hash.ToLowerInvariant(), $relativePath
}

Set-Content -Path $outputPath -Value $lines -NoNewline:$false
Write-Host "[Wevoa] Wrote checksums to $outputPath"
