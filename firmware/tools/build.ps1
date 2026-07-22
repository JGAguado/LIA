<#
.SYNOPSIS
    Builds a LIA Meshtastic firmware variant.

.DESCRIPTION
    Clones meshtastic/firmware (if not already present) at the pinned release
    tag, copies this repo's board overlay on top of it, and runs `pio run`.
    Must be run from native PowerShell, not Git Bash/MSYS -- the ESP-IDF
    toolchain PlatformIO pulls in for this platform refuses to run under MSYS.

.PARAMETER Variant
    Which board overlay to build: "lia_v1" (default) or "lia_v2" (IMU
    removed, I2C battery gauge enabled -- see
    firmware/meshtastic/variants/esp32s3/lia_v2/variant.h).

.PARAMETER CheckoutPath
    Where to clone/find the Meshtastic firmware checkout. Defaults to a
    sibling directory of this repo.

.PARAMETER Tag
    Meshtastic firmware release tag to build against.

.PARAMETER Upload
    If set, also flashes the board after a successful build (requires -Port).

.PARAMETER Port
    Serial port to flash, e.g. COM7. Only used with -Upload.
#>
param(
    [ValidateSet("lia_v1", "lia_v2")]
    [string]$Variant = "lia_v1",
    [string]$CheckoutPath = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "meshtastic-firmware"),
    [string]$Tag = "v2.7.26.54e0d8d",
    [switch]$Upload,
    [string]$Port
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$FirmwareDir = Join-Path $RepoRoot "firmware"
$Overlay = Join-Path $FirmwareDir "meshtastic"

if (-not (Test-Path $CheckoutPath)) {
    Write-Host "Cloning meshtastic/firmware @ $Tag into $CheckoutPath"
    git -c core.longpaths=true clone --recurse-submodules --depth 1 --branch $Tag `
        https://github.com/meshtastic/firmware.git $CheckoutPath
} else {
    Write-Host "Using existing checkout at $CheckoutPath"
}

Write-Host "Copying $Variant overlay"

Copy-Item (Join-Path $Overlay "boards\$Variant.json") (Join-Path $CheckoutPath "boards\") -Force

$VariantDest = Join-Path $CheckoutPath "variants\esp32s3\$Variant"
New-Item -ItemType Directory -Force $VariantDest | Out-Null
Copy-Item (Join-Path $Overlay "variants\esp32s3\$Variant\*") $VariantDest -Force

$ExtraVariantDest = Join-Path $CheckoutPath "src\platform\extra_variants\$Variant"
New-Item -ItemType Directory -Force $ExtraVariantDest | Out-Null
Copy-Item (Join-Path $Overlay "extra_variants\$Variant\*") $ExtraVariantDest -Force

$LiaSrcDest = Join-Path $CheckoutPath "src\lia"
New-Item -ItemType Directory -Force $LiaSrcDest | Out-Null
Copy-Item (Join-Path $FirmwareDir "board\*.h") $LiaSrcDest -Force
Copy-Item (Join-Path $FirmwareDir "board\*.cpp") $LiaSrcDest -Force

$ServicesSrcDest = Join-Path $CheckoutPath "src\lia\services"
New-Item -ItemType Directory -Force $ServicesSrcDest | Out-Null
Copy-Item (Join-Path $FirmwareDir "services\*.h") $ServicesSrcDest -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $FirmwareDir "services\*.cpp") $ServicesSrcDest -Force -ErrorAction SilentlyContinue

Push-Location $CheckoutPath
try {
    if ($Upload) {
        if (-not $Port) {
            throw "-Port is required with -Upload"
        }
        pio run -e $Variant -t upload --upload-port $Port
    } else {
        pio run -e $Variant
    }
} finally {
    Pop-Location
}
