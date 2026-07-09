# VocalSuiteUltraPro clean build helper
# Close locked Standalone/VST host/build processes before deleting Build.

$ErrorActionPreference = "SilentlyContinue"

$processes = @(
    "VocalSuiteUltraPro",
    "devenv",
    "MSBuild",
    "cmake",
    "VST3PluginTestHost",
    "AudioPluginHost",
    "vst3host"
)

foreach ($p in $processes) {
    Get-Process $p | Stop-Process -Force
}

Start-Sleep -Seconds 1

if (Test-Path ".\Build") {
    Remove-Item ".\Build" -Recurse -Force
}

cmake -S . -B Build -G "Visual Studio 18 2026" -A x64
cmake --build Build --config Release
