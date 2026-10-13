param([string]$ZigPath = '',[string]$Python = 'python',[string]$GameExe = '')
$ErrorActionPreference = 'Stop'
if (!$ZigPath) { $ZigPath = Join-Path $PSScriptRoot '.tools/zig-x86_64-windows-0.15.2/zig.exe' }
$ZigPath = (Resolve-Path -LiteralPath $ZigPath).Path
if ((& $ZigPath version).Trim() -ne '0.15.2') { throw 'Requires Zig 0.15.2' }
if ($GameExe) {
 & $Python (Join-Path $PSScriptRoot 'scripts/verify_game_sites.py') $GameExe
 if ($LASTEXITCODE) { throw 'Game hook signature verification failed' }
}
$captureFlags = @('-DFRAME_CAPTURE')
$build = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $build | Out-Null
$source = Join-Path $PSScriptRoot 'src/PlayerObstructionHoldFire.c'
& $ZigPath cc -g0 -fms-extensions -target x86_64-windows-gnu -shared @captureFlags -O2 -Wall -Wextra -Werror $source -lbcrypt -o (Join-Path $build 'PlayerObstructionHoldFire.asi')
if ($LASTEXITCODE) { throw 'ASI build failed' }
& $Python (Join-Path $PSScriptRoot 'scripts/normalize_pe.py') (Join-Path $build 'PlayerObstructionHoldFire.asi')
if ($LASTEXITCODE) { throw 'PE normalization failed' }
foreach ($kind in @('GEOMETRY_TEST', 'NATIVE_TEST')) {
 $test = Join-Path $build "$kind.exe"
 & $ZigPath cc -g0 -fms-extensions -target x86_64-windows-gnu -O2 @captureFlags "-D$kind" $source -lbcrypt -o $test
 if ($LASTEXITCODE) { throw 'Test build failed' }
 & $test
 if ($LASTEXITCODE) { throw "$kind failed" }
}

& {
 $test = Join-Path $build 'FRAME_TEST.exe'
 & $ZigPath cc -g0 -fms-extensions -target x86_64-windows-gnu -O2 -DFRAME_CAPTURE -DFRAME_TEST $source -lbcrypt -o $test
 if ($LASTEXITCODE) { throw 'Frame capture test build failed' }
 & $test
 if ($LASTEXITCODE) { throw 'Frame capture test failed' }
}

& (Join-Path $PSScriptRoot 'scripts/get-loader.ps1')
& $Python (Join-Path $PSScriptRoot 'scripts/package.py')
if ($LASTEXITCODE) { throw 'Packaging failed' }


