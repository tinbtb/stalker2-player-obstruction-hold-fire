$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$folder = Join-Path $root '.tools'
$zip = Join-Path $folder 'zig-x86_64-windows-0.15.2.zip'
$expected = '3a0ed1e8799a2f8ce2a6e6290a9ff22e6906f8227865911fb7ddedc3cc14cb0c'
New-Item -ItemType Directory -Force $folder | Out-Null
if (!(Test-Path -LiteralPath $zip)) {
    Invoke-WebRequest 'https://ziglang.org/download/0.15.2/zig-x86_64-windows-0.15.2.zip' -OutFile $zip
}
if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'Compiler archive SHA256 mismatch; refusing to extract. Remove the archive and retry.'
}
Expand-Archive -LiteralPath $zip -DestinationPath $folder -Force
$zig = Join-Path $folder 'zig-x86_64-windows-0.15.2\zig.exe'
& $zig version
if ($LASTEXITCODE -ne 0) { throw 'Compiler cannot run' }
Write-Output "Verified compiler: $zig"
