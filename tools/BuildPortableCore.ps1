param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Win32', 'x64')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer was not found. Install Visual Studio 2022 with Desktop development with C++.'
}

$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    throw 'A compatible MSBuild installation was not found.'
}

& $msbuild (Join-Path $repositoryRoot 'PortableCore.sln') /m /nologo /v:minimal "/p:Configuration=$Configuration" "/p:Platform=$Platform"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$outputDirectory = if ($Platform -eq 'x64') { Join-Path $repositoryRoot "x64\$Configuration" } else { Join-Path $repositoryRoot $Configuration }
& (Join-Path $outputDirectory 'PortableCoreTests.exe')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $repositoryRoot 'tests\RunEdidInspectTests.ps1') -Executable (Join-Path $outputDirectory 'EdidInspect.exe')

Write-Output "Portable Core build and tests passed: $Configuration|$Platform"
