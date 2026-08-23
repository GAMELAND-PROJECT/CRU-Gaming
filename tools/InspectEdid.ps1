param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$EdidPath,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Win32', 'x64')]
    [string]$Platform = 'x64',
    [switch]$Json,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$resolvedEdid = (Resolve-Path -LiteralPath $EdidPath).Path
$outputDirectory = if ($Platform -eq 'x64') { Join-Path $repositoryRoot "x64\$Configuration" } else { Join-Path $repositoryRoot $Configuration }
$executable = Join-Path $outputDirectory 'EdidInspect.exe'

if (-not (Test-Path -LiteralPath $executable)) {
    throw "EdidInspect has not been built at '$executable'. Run tools\BuildPortableCore.ps1 first."
}

$arguments = @()
if ($Json) { $arguments += '--json' }
$arguments += $resolvedEdid
$result = & $executable @arguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($OutputPath) {
    $resolvedOutput = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
    $result | Set-Content -LiteralPath $resolvedOutput -Encoding utf8
    Write-Output "Report written to: $resolvedOutput"
} else {
    $result
}
