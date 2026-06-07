[CmdletBinding()]
param(
    [string]$Client = "rof",
    [string]$PayloadPath = "",
    [string]$OutputPath = "",
    [string]$BaseUrl = "http://localhost:8080/patcher/",
    [string]$PatcherExe = "",
    [string]$PatcherFileName = "eqemupatcher",
    [string]$Version = "",
    [string]$PatchNotesPath = "",
    [string]$StatusPath = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

function Get-Md5([string]$Path) {
    (Get-FileHash -Algorithm MD5 -LiteralPath $Path).Hash.ToUpperInvariant()
}

function ConvertTo-ForwardSlash([string]$Path) {
    $Path.Replace("\", "/")
}

function ConvertTo-YamlSingleQuoted([string]$Value) {
    "'" + $Value.Replace("'", "''") + "'"
}

function Write-Utf8NoBom([string]$Path, [string]$Value) {
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Value, $encoding)
}

if ([string]::IsNullOrWhiteSpace($PayloadPath)) {
    $PayloadPath = Join-Path $PSScriptRoot "..\$Client"
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $PSScriptRoot "publish"
}
if ([string]::IsNullOrWhiteSpace($PatchNotesPath)) {
    $PatchNotesPath = Join-Path $PayloadPath "patch_notes.txt"
}
if ([string]::IsNullOrWhiteSpace($StatusPath)) {
    $StatusPath = Join-Path $PayloadPath "patcher_status.yml"
}
if (-not $BaseUrl.EndsWith("/")) {
    $BaseUrl += "/"
}

$payloadRoot = [System.IO.Path]::GetFullPath($PayloadPath)
$outputRoot = [System.IO.Path]::GetFullPath($OutputPath)
$clientOutput = Join-Path $outputRoot $Client

if (-not (Test-Path -LiteralPath $payloadRoot)) {
    throw "Payload path does not exist: $payloadRoot"
}

if ($Clean -and (Test-Path -LiteralPath $outputRoot)) {
    if ((Split-Path -Leaf $outputRoot) -ne "publish") {
        throw "Refusing to recursively delete an output path that is not named 'publish': $outputRoot"
    }
    Remove-Item -LiteralPath $outputRoot -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $clientOutput | Out-Null

$metadataNames = @(
    "delete.txt",
    "filelist_$Client.yml",
    "filelistbuilder.yml",
    "patch.zip",
    "patch_notes.txt",
    "patch-notes.txt",
    "patcher_status.yml",
    "status.yml",
    "$PatcherFileName-hash.txt",
    "$PatcherFileName.exe"
)

$downloads = @()
$payloadFiles = Get-ChildItem -LiteralPath $payloadRoot -Recurse -File | Where-Object {
    $metadataNames -notcontains $_.Name
}

foreach ($file in $payloadFiles) {
    $relative = $file.FullName.Substring($payloadRoot.Length).TrimStart("\", "/")
    $relativeUrl = ConvertTo-ForwardSlash $relative
    $target = Join-Path $clientOutput $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force

    $downloads += [pscustomobject]@{
        Name = $relativeUrl
        Md5 = Get-Md5 $file.FullName
        Size = $file.Length
        Date = $file.LastWriteTimeUtc.ToString("yyyy-MM-ddTHH:mm:ssZ")
    }
}

$deletes = @()
$deleteFile = Join-Path $payloadRoot "delete.txt"
if (Test-Path -LiteralPath $deleteFile) {
    $deletes = @(Get-Content -LiteralPath $deleteFile | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith("#") })
}

$version = if ([string]::IsNullOrWhiteSpace($Version)) {
    (Get-Date).ToUniversalTime().ToString("yyyyMMddHHmmss")
}
else {
    $Version.Trim()
}
$fileListPath = Join-Path $clientOutput "filelist_$Client.yml"
$yaml = New-Object System.Text.StringBuilder
[void]$yaml.AppendLine("version: '$version'")
[void]$yaml.AppendLine("downloadprefix: '${BaseUrl}${Client}/'")
[void]$yaml.AppendLine("downloads:")
foreach ($item in $downloads | Sort-Object Name) {
    [void]$yaml.AppendLine("  - name: $(ConvertTo-YamlSingleQuoted $item.Name)")
    [void]$yaml.AppendLine("    md5: $(ConvertTo-YamlSingleQuoted $item.Md5)")
    [void]$yaml.AppendLine("    date: $(ConvertTo-YamlSingleQuoted $item.Date)")
    [void]$yaml.AppendLine("    size: $($item.Size)")
}
if ($deletes.Count -gt 0) {
    [void]$yaml.AppendLine("deletes:")
    foreach ($delete in $deletes) {
        [void]$yaml.AppendLine("  - name: $(ConvertTo-YamlSingleQuoted $delete)")
    }
}
Write-Utf8NoBom $fileListPath $yaml.ToString()

if (Test-Path -LiteralPath $PatchNotesPath) {
    Copy-Item -LiteralPath $PatchNotesPath -Destination (Join-Path $outputRoot "patch_notes.txt") -Force
    Copy-Item -LiteralPath $PatchNotesPath -Destination (Join-Path $clientOutput "patch_notes.txt") -Force
}

if (Test-Path -LiteralPath $StatusPath) {
    Copy-Item -LiteralPath $StatusPath -Destination (Join-Path $outputRoot "patcher_status.yml") -Force
    Copy-Item -LiteralPath $StatusPath -Destination (Join-Path $clientOutput "patcher_status.yml") -Force
}

if (-not [string]::IsNullOrWhiteSpace($PatcherExe)) {
    $patcherExePath = [System.IO.Path]::GetFullPath($PatcherExe)
    if (-not (Test-Path -LiteralPath $patcherExePath)) {
        throw "Patcher executable does not exist: $patcherExePath"
    }
    $publishedExe = Join-Path $outputRoot "$PatcherFileName.exe"
    Copy-Item -LiteralPath $patcherExePath -Destination $publishedExe -Force
    Write-Utf8NoBom (Join-Path $outputRoot "$PatcherFileName-hash.txt") (Get-Md5 $publishedExe)
}

$manifest = @"
service: eqemu-patcher
client: $Client
version: $(ConvertTo-YamlSingleQuoted $version)
baseUrl: $BaseUrl
fileList: ${Client}/filelist_$Client.yml
patchNotes: patch_notes.txt
status: patcher_status.yml
downloads: $($downloads.Count)
bytes: $(($downloads | Measure-Object -Property Size -Sum).Sum)
generatedUtc: $((Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ"))
"@
Write-Utf8NoBom (Join-Path $outputRoot "service_manifest.yml") $manifest

$index = @"
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>EQEmu Patcher Service</title>
  <style>
    body { margin: 0; font-family: Segoe UI, Arial, sans-serif; background: #111720; color: #e5ebf3; }
    main { max-width: 900px; margin: 48px auto; padding: 0 24px; }
    h1 { font-size: 34px; margin-bottom: 6px; }
    a { color: #e0b55c; }
    .grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; margin-top: 24px; }
    .card { background: #1c2330; border: 1px solid #344155; padding: 18px; border-radius: 6px; }
    .muted { color: #aab5c4; }
  </style>
</head>
<body>
  <main>
    <h1>EQEmu Patcher Service</h1>
    <p class="muted">Patch stream <strong>$version</strong> for <strong>$Client</strong>.</p>
    <section class="grid">
      <div class="card"><h2>Manifest</h2><p><a href="$Client/filelist_$Client.yml">$Client/filelist_$Client.yml</a></p></div>
      <div class="card"><h2>Patch Notes</h2><p><a href="patch_notes.txt">patch_notes.txt</a></p></div>
      <div class="card"><h2>Status</h2><p><a href="patcher_status.yml">patcher_status.yml</a></p></div>
      <div class="card"><h2>Downloads</h2><p>$($downloads.Count) files, $(($downloads | Measure-Object -Property Size -Sum).Sum) bytes</p></div>
    </section>
  </main>
</body>
</html>
"@
Write-Utf8NoBom (Join-Path $outputRoot "index.html") $index

Write-Host "Patcher release generated at $outputRoot" -ForegroundColor Green
Write-Host "File list: $fileListPath"
Write-Host "Base URL: $BaseUrl"
