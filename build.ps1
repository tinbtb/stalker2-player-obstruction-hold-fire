param([string]$ZigPath = '', [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
if (!$ZigPath) { $ZigPath = Join-Path $root '.tools\zig-x86_64-windows-0.15.2\zig.exe' }
if (!(Test-Path -LiteralPath $ZigPath)) { throw 'Run scripts/get-zig.ps1 first or supply -ZigPath.' }
$ZigPath = (Resolve-Path -LiteralPath $ZigPath).Path
$version = & $ZigPath version
if ($LASTEXITCODE -ne 0 -or $version.Trim() -ne '0.15.2') { throw 'This build requires Zig 0.15.2.' }
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Force $build | Out-Null
$source = Join-Path $root 'src\PlayerObstructionHoldFire.c'
$dll = Join-Path $build 'PlayerObstructionHoldFire.dll'
& $ZigPath cc -target x86_64-windows-gnu -shared -O2 -Wall -Wextra $source -lbcrypt -o $dll
if ($LASTEXITCODE -ne 0) { throw 'DLL build failed' }
$test = Join-Path $build 'geometry-test.exe'
& $ZigPath cc -target x86_64-windows-gnu -O2 -DGEOMETRY_TEST $source -lbcrypt -o $test
if ($LASTEXITCODE -ne 0) { throw 'Geometry test build failed' }
& $test
if ($LASTEXITCODE -ne 0) { throw 'Geometry tests failed' }
$bridgeTest = Join-Path $build 'bridge-test.exe'
& $ZigPath cc -target x86_64-windows-gnu -O2 -DBRIDGE_TEST $source -lbcrypt -o $bridgeTest
if ($LASTEXITCODE -ne 0) { throw 'Bridge test build failed' }
& $bridgeTest
if ($LASTEXITCODE -ne 0) { throw 'Bridge tests failed' }
& $Python (Join-Path $root 'scripts\package.py')
if ($LASTEXITCODE -ne 0) { throw 'Verification/packaging failed' }
