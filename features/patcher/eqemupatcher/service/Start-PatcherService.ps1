[CmdletBinding()]
param(
    [Alias("RootPath")]
    [string]$Root = "",
    [Alias("Prefix")]
    [string]$UrlPrefix = "http://localhost:8080/patcher/"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = Join-Path $PSScriptRoot "publish"
}
if (-not $UrlPrefix.EndsWith("/")) {
    $UrlPrefix += "/"
}

$rootPath = [System.IO.Path]::GetFullPath($Root)
if (-not (Test-Path -LiteralPath $rootPath)) {
    throw "Publish root does not exist: $rootPath"
}
$rootPrefix = $rootPath.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar

$listener = [System.Net.HttpListener]::new()
$listener.Prefixes.Add($UrlPrefix)
$listener.Start()

$uriPrefixForPath = $UrlPrefix -replace "^([^:]+://)[+*](?=[:/])", "`$1localhost"
$prefixPath = ([uri]$uriPrefixForPath).AbsolutePath.TrimEnd("/")
Write-Host "Serving $rootPath at $UrlPrefix" -ForegroundColor Green
Write-Host "Press Ctrl+C to stop."

function Get-ContentType([string]$Path) {
    switch ([System.IO.Path]::GetExtension($Path).ToLowerInvariant()) {
        ".html" { "text/html; charset=utf-8"; break }
        ".htm" { "text/html; charset=utf-8"; break }
        ".txt" { "text/plain; charset=utf-8"; break }
        ".yml" { "text/yaml; charset=utf-8"; break }
        ".yaml" { "text/yaml; charset=utf-8"; break }
        ".json" { "application/json; charset=utf-8"; break }
        ".zip" { "application/zip"; break }
        ".exe" { "application/octet-stream"; break }
        ".png" { "image/png"; break }
        ".ico" { "image/x-icon"; break }
        default { "application/octet-stream" }
    }
}

try {
    while ($listener.IsListening) {
        $context = $listener.GetContext()
        try {
            $requestPath = [uri]::UnescapeDataString($context.Request.Url.AbsolutePath)
            if ($prefixPath -and $requestPath.StartsWith($prefixPath, [System.StringComparison]::OrdinalIgnoreCase)) {
                $requestPath = $requestPath.Substring($prefixPath.Length)
            }
            $relative = $requestPath.TrimStart("/")
            if ([string]::IsNullOrWhiteSpace($relative)) {
                $relative = "index.html"
            }
            $relative = $relative.Replace("/", "\")
            $candidate = [System.IO.Path]::GetFullPath((Join-Path $rootPath $relative))
            if (-not ($candidate + [System.IO.Path]::DirectorySeparatorChar).StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase) -and
                -not $candidate.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                $context.Response.StatusCode = 403
                $context.Response.Close()
                continue
            }
            if (Test-Path -LiteralPath $candidate -PathType Container) {
                $candidate = Join-Path $candidate "index.html"
            }
            if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
                $context.Response.StatusCode = 404
                $context.Response.Close()
                continue
            }

            $bytes = [System.IO.File]::ReadAllBytes($candidate)
            $context.Response.ContentType = Get-ContentType $candidate
            $context.Response.ContentLength64 = $bytes.Length
            $context.Response.Headers["Cache-Control"] = "no-cache"
            if ($context.Request.HttpMethod -ne "HEAD") {
                try {
                    $context.Response.OutputStream.Write($bytes, 0, $bytes.Length)
                }
                catch {
                    Write-Warning $_.Exception.Message
                }
            }
            try {
                $context.Response.Close()
            }
            catch {
                Write-Warning $_.Exception.Message
            }
        }
        catch {
            Write-Warning $_.Exception.Message
            if ($context.Response) {
                try {
                    $context.Response.Close()
                }
                catch {
                    Write-Warning $_.Exception.Message
                }
            }
        }
    }
}
finally {
    $listener.Stop()
    $listener.Close()
}
