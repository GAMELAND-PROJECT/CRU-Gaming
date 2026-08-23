param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Continue'
$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$fixturePath = Join-Path $PSScriptRoot 'fixtures\edid-1080p60.hex'
$hex = (Get-Content -Raw -LiteralPath $fixturePath) -split '\s+' | Where-Object { $_ }
$bytes = [byte[]]($hex | ForEach-Object { [Convert]::ToByte($_, 16) })

if ($bytes.Length -ne 128) {
    throw "Fixture must contain exactly 128 bytes; got $($bytes.Length)."
}

$validPath = [IO.Path]::GetTempFileName()
$badChecksumPath = [IO.Path]::GetTempFileName()
$badSizePath = [IO.Path]::GetTempFileName()

try {
    [IO.File]::WriteAllBytes($validPath, $bytes)
    $output = & $executablePath $validPath 2>&1
    $text = $output -join [Environment]::NewLine
    if ($LASTEXITCODE -ne 0 -or $text -notmatch '1920x1080p @ 60\.000 Hz') {
        throw "Valid EDID inspection failed:`n$output"
    }

    $badChecksum = [byte[]]$bytes.Clone()
    $badChecksum[127] = $badChecksum[127] -bxor 1
    [IO.File]::WriteAllBytes($badChecksumPath, $badChecksum)
    $output = & $executablePath $badChecksumPath 2>&1
    $text = $output -join [Environment]::NewLine
    if ($LASTEXITCODE -eq 0 -or $text -notmatch 'invalid EDID') {
        throw "Bad-checksum EDID was not rejected:`n$output"
    }

    [IO.File]::WriteAllBytes($badSizePath, $bytes[0..126])
    $output = & $executablePath $badSizePath 2>&1
    $text = $output -join [Environment]::NewLine
    if ($LASTEXITCODE -eq 0 -or $text -notmatch 'multiple of 128') {
        throw "Bad-size EDID was not rejected:`n$output"
    }

    Write-Output 'EdidInspect smoke tests passed.'
}
finally {
    Remove-Item -LiteralPath $validPath, $badChecksumPath, $badSizePath -Force -ErrorAction SilentlyContinue
}
