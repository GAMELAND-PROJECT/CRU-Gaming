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
$ctaPath = [IO.Path]::GetTempFileName()

try {
    [IO.File]::WriteAllBytes($validPath, $bytes)
    $output = & $executablePath $validPath 2>&1
    $text = $output -join [Environment]::NewLine
    if ($LASTEXITCODE -ne 0 -or $text -notmatch '1920x1080p @ 60\.000 Hz' -or $text -notmatch 'Consistent timings: 1' -or $text -notmatch 'Inconsistent timings: 0' -or $text -notmatch 'Internal consistency: OK') {
        throw "Valid EDID inspection failed:`n$output"
    }

    $baseWithExtension = [byte[]]$bytes.Clone()
    $baseWithExtension[126] = 1
    $baseWithExtension[127] = 0
    $sum = 0
    $baseWithExtension | ForEach-Object { $sum += $_ }
    $baseWithExtension[127] = [byte]((256 - ($sum % 256)) % 256)
    $cta = [byte[]]::new(128)
    $cta[0] = 2
    $cta[1] = 3
    $cta[2] = 7
    $cta[4] = 0x42
    $cta[5] = 16
    $cta[6] = 31
    [Array]::Copy($bytes, 54, $cta, 7, 18)
    $sum = 0
    $cta | ForEach-Object { $sum += $_ }
    $cta[127] = [byte]((256 - ($sum % 256)) % 256)
    $fullEdid = [byte[]]::new(256)
    [Array]::Copy($baseWithExtension, 0, $fullEdid, 0, 128)
    [Array]::Copy($cta, 0, $fullEdid, 128, 128)
    [IO.File]::WriteAllBytes($ctaPath, $fullEdid)
    $output = & $executablePath $ctaPath 2>&1
    $text = $output -join [Environment]::NewLine
    if ($LASTEXITCODE -ne 0 -or $text -notmatch 'CTA advertised video modes: 2' -or $text -notmatch 'VIC 16: 1920x1080p @ 60 Hz') {
        throw "CTA EDID inspection failed:`n$output"
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
    Remove-Item -LiteralPath $validPath, $badChecksumPath, $badSizePath, $ctaPath -Force -ErrorAction SilentlyContinue
}
