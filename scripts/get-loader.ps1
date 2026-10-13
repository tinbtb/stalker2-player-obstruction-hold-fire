$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$meta = Get-Content -Raw (Join-Path $root 'release.json') | ConvertFrom-Json
$folder = Join-Path $root '.tools/asi-loader'
New-Item -ItemType Directory -Force $folder | Out-Null
$zip = Join-Path $folder 'upstream.zip'
$dll = Join-Path $folder 'winhttp.dll'
if (!(Test-Path -LiteralPath $zip)) { Invoke-WebRequest $meta.loader_url -OutFile $zip }
if ((Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant() -ne $meta.loader_archive_sha256) { throw 'Loader archive SHA256 mismatch' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($zip)
try {
 $entry = $archive.GetEntry('dinput8.dll')
 if (!$entry) { throw 'Official loader DLL absent from archive' }
 [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dll, $true)
} finally { $archive.Dispose() }
if ((Get-FileHash -LiteralPath $dll).Hash.ToLowerInvariant() -ne $meta.loader_dll_sha256) { throw 'Loader DLL SHA256 mismatch' }
Write-Output 'Official Ultimate ASI Loader 9.7.4 verified.'
