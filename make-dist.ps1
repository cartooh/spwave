<#
.SYNOPSIS
    ビルド済み spwave.exe とプラグインを dist\ に集めて、リポジトリに登録できる形にする。

.DESCRIPTION
    spwave\x64\Release でビルドされた spwave.exe と plugins\ を dist\win-x64\ に
    コピーする。dist\ はそのまま実行可能な配布イメージで、git 管理対象。

.EXAMPLE
    .\make-dist.ps1
#>
param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$root     = $PSScriptRoot
$buildDir = Join-Path $root "spwave\$Platform\$Configuration"
$exe      = Join-Path $buildDir "spwave.exe"
$distDir  = Join-Path $root "dist\win-$Platform"

if (-not (Test-Path $exe)) {
    throw "spwave.exe not found: $exe  (build first: MSBuild.exe spwave\spwave.vcxproj /p:Configuration=$Configuration /p:Platform=$Platform)"
}
if (-not (Test-Path (Join-Path $buildDir "plugins"))) {
    throw "plugins folder not found in $buildDir  (run setup-deps.ps1 first)"
}

New-Item -ItemType Directory -Force $distDir | Out-Null
Copy-Item -Force $exe $distDir
Remove-Item -Recurse -Force -ErrorAction Ignore (Join-Path $distDir "plugins")
Copy-Item -Recurse -Force (Join-Path $buildDir "plugins") (Join-Path $distDir "plugins")

$ver = (Get-Item $exe).VersionInfo.FileVersion
Write-Host "Done: $distDir (spwave.exe $ver + $((Get-ChildItem (Join-Path $distDir 'plugins')).Count) plugin files)" -ForegroundColor Green
