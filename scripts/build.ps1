$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root
try {
    cmake --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
    cmake --build --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
    Write-Host "Build finished. Search for VirtuStreamCamera.exe under $Root\out\build."
} finally {
    Pop-Location
}
