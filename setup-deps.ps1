<#
.SYNOPSIS
    spwave のビルドに必要な spLibs バイナリをダウンロード・展開・配置する。

.DESCRIPTION
    公式アーカイブ (https://www-ie.meijo-u.ac.jp/~banno/archive/) から
    spBase / spLib / spAudio / spComponent のバイナリ版と spPlugin (win64) を
    取得し、spwave.vcxproj が参照する include/ と lib/ に配置する。
    実行時プラグインは spwave\x64\Release\plugins に配置する。

    ダウンロードしたzipは .deps\ にキャッシュされ、2回目以降は再利用される。

.EXAMPLE
    .\setup-deps.ps1            # 依存関係を取得・配置
    .\setup-deps.ps1 -Clean     # include/ lib/ .deps/ を削除してから取得し直す

    バージョンを変える場合:
    .\setup-deps.ps1 -SpBaseVer 0.8.26-1
#>
param(
    [string]$SpBaseVer      = "0.8.25-1",
    [string]$SpLibVer       = "0.9.5-1",
    [string]$SpAudioVer     = "0.7.16-4",
    [string]$SpComponentVer = "0.6.23-1",
    [string]$SpPluginVer    = "0.8.6-4",
    [string]$ArchiveUrl     = "https://www-ie.meijo-u.ac.jp/~banno/archive",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root    = $PSScriptRoot
$depsDir = Join-Path $root ".deps"
$srcDir  = Join-Path $depsDir "extracted"

if ($Clean) {
    Write-Host "== Cleaning previous dependencies" -ForegroundColor Cyan
    Remove-Item -Recurse -Force -ErrorAction Ignore `
        $depsDir, (Join-Path $root "include"), (Join-Path $root "lib")
}

$archives = @(
    "spBase-$SpBaseVer.bin.zip",
    "spLib-$SpLibVer.bin.zip",
    "spAudio-$SpAudioVer.bin.zip",
    "spComponent-$SpComponentVer.bin.zip",
    "spPlugin-$SpPluginVer.win64.zip"
)

New-Item -ItemType Directory -Force $depsDir | Out-Null

# --- Download (cached in .deps) ---
foreach ($zip in $archives) {
    $out = Join-Path $depsDir $zip
    if (Test-Path $out) {
        Write-Host "== $zip (cached)" -ForegroundColor DarkGray
        continue
    }
    Write-Host "== Downloading $zip" -ForegroundColor Cyan
    curl.exe -fsSL -o $out "$ArchiveUrl/$zip"
    if ($LASTEXITCODE -ne 0) { throw "Download failed: $ArchiveUrl/$zip" }
}

# --- Extract (Windows bsdtar: fast and handles zip) ---
Remove-Item -Recurse -Force -ErrorAction Ignore $srcDir
New-Item -ItemType Directory -Force $srcDir | Out-Null
foreach ($zip in $archives) {
    Write-Host "== Extracting $zip" -ForegroundColor Cyan
    & "$env:SystemRoot\System32\tar.exe" -xf (Join-Path $depsDir $zip) -C $srcDir
    if ($LASTEXITCODE -ne 0) { throw "Extract failed: $zip" }
}

# --- Place headers and import libraries ---
# spwave.vcxproj references ..\include and ..\lib\$(Platform)\$(PlatformToolset)
$incDest = Join-Path $root "include\sp"
$libDest = Join-Path $root "lib"
New-Item -ItemType Directory -Force $incDest, $libDest | Out-Null

foreach ($dir in Get-ChildItem $srcDir -Directory | Where-Object Name -notlike "spPlugin*") {
    Write-Host "== Installing $($dir.Name) into include/ and lib/" -ForegroundColor Cyan
    Copy-Item -Recurse -Force (Join-Path $dir.FullName "include\sp\*") $incDest
    # Windows 用の .lib ツリー (v143, x64, ARM64, ...) のみコピー。mac/iOS の .a は除外
    foreach ($sub in Get-ChildItem (Join-Path $dir.FullName "lib") -Directory) {
        Copy-Item -Recurse -Force $sub.FullName $libDest
    }
}

# --- Place runtime plugins next to the build output ---
$pluginSrc  = Get-ChildItem $srcDir -Directory | Where-Object Name -like "spPlugin*" | Select-Object -First 1
$pluginDest = Join-Path $root "spwave\x64\Release\plugins"
Write-Host "== Installing plugins into spwave\x64\Release\plugins" -ForegroundColor Cyan
New-Item -ItemType Directory -Force $pluginDest | Out-Null
Copy-Item -Recurse -Force (Join-Path $pluginSrc.FullName "plugins\*") $pluginDest

Write-Host ""
Write-Host "Done. Next step:" -ForegroundColor Green
Write-Host '  MSBuild.exe spwave\spwave.vcxproj /p:Configuration=Release /p:Platform=x64'
