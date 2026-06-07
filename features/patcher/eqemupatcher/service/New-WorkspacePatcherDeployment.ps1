[CmdletBinding()]
param(
    [string]$WorkspaceRoot = "D:\Codex\Apps\EQEmu-feature-workspaces",
    [string[]]$Project = @(),
    [string]$Client = "rof",
    [string]$BaseUrl = "http://localhost:8091/patcher/",
    [string]$OutputPath = "",
    [string]$PayloadRoot = "",
    [string]$PatcherFileName = "eqemupatcher",
    [string]$LoginHost = "",
    [int]$LoginPort = 0,
    [switch]$Clean,
    [switch]$SkipLauncherBuild,
    [switch]$SkipClientInstall,
    [switch]$AllowMissingClientFiles,
    [switch]$SkipEQUIXml,
    [switch]$NoAutoIncrementPatchVersion,
    [switch]$ProjectOnlyWorkspaceManifest,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Get-FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path)
}

function Test-PathChildOf {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Root
    )

    $fullPath = (Get-FullPath $Path).TrimEnd("\", "/")
    $fullRoot = (Get-FullPath $Root).TrimEnd("\", "/")
    $rootWithSlash = $fullRoot + [System.IO.Path]::DirectorySeparatorChar
    return $fullPath.Equals($fullRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        $fullPath.StartsWith($rootWithSlash, [System.StringComparison]::OrdinalIgnoreCase)
}

function Reset-Directory {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$ExpectedRoot,
        [switch]$DryRun
    )

    $fullPath = Get-FullPath $Path
    $fullRoot = Get-FullPath $ExpectedRoot
    if (-not (Test-PathChildOf -Path $fullPath -Root $fullRoot)) {
        throw "Refusing to reset a path outside the expected root: $fullPath"
    }

    if ($DryRun) {
        Write-Host "Would reset directory: $fullPath"
        return
    }

    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $fullPath | Out-Null
}

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Value
    )

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Value, $encoding)
}

function ConvertTo-ForwardSlash([string]$Path) {
    return $Path.Replace("\", "/")
}

function ConvertTo-NativeSlash([string]$Path) {
    return $Path.Replace("/", [string][System.IO.Path]::DirectorySeparatorChar)
}

function Join-NativePath {
    param(
        [Parameter(Mandatory = $true)] [string]$Root,
        [Parameter(Mandatory = $true)] [string]$RelativePath
    )

    return Join-Path $Root (ConvertTo-NativeSlash $RelativePath)
}

function ConvertTo-Html {
    param([string]$Value = "")
    return [System.Net.WebUtility]::HtmlEncode($Value)
}

function ConvertTo-YamlSingleQuoted {
    param([string]$Value = "")
    return "'$($Value.Replace("'", "''"))'"
}

function Normalize-ClientDestination {
    param([string]$Path = "")

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    return (ConvertTo-ForwardSlash (ConvertTo-NativeSlash $Path)).TrimStart("/", "\")
}

function Import-YamlDotNet {
    $assemblyLoaded = [AppDomain]::CurrentDomain.GetAssemblies() | Where-Object {
        $_.GetName().Name -eq "YamlDotNet"
    } | Select-Object -First 1
    if ($null -ne $assemblyLoaded) {
        return
    }

    $yamlDll = Join-Path $PSScriptRoot "..\EQEmu Patcher\packages\YamlDotNet.5.3.0\lib\net45\YamlDotNet.dll"
    if (-not (Test-Path -LiteralPath $yamlDll)) {
        throw "YamlDotNet was not found: $yamlDll. Restore the patcher NuGet packages first."
    }
    Add-Type -Path $yamlDll
}

function ConvertTo-PlainObject {
    param([object]$Value)

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $result[[string]$key] = ConvertTo-PlainObject $Value[$key]
        }
        return [pscustomobject]$result
    }

    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        $items = New-Object System.Collections.Generic.List[object]
        foreach ($item in $Value) {
            [void]$items.Add((ConvertTo-PlainObject $item))
        }
        return $items.ToArray()
    }

    return $Value
}

function Read-YamlFile {
    param([Parameter(Mandatory = $true)] [string]$Path)

    Import-YamlDotNet
    $reader = [System.IO.StreamReader]::new($Path)
    try {
        $builder = [YamlDotNet.Serialization.DeserializerBuilder]::new()
        $deserializer = $builder.Build()
        return ConvertTo-PlainObject ($deserializer.Deserialize($reader))
    }
    finally {
        $reader.Dispose()
    }
}

function Resolve-ProjectPatcherConfig {
    param([Parameter(Mandatory = $true)] [object]$Feature)

    $candidates = @(
        (Join-NativePath -Root $Feature.path -RelativePath "features/$($Feature.id)/patcher.yml"),
        (Join-NativePath -Root $Feature.path -RelativePath "features/$($Feature.id)/patcher.yaml"),
        (Join-NativePath -Root $Feature.path -RelativePath "patcher.yml"),
        (Join-NativePath -Root $Feature.path -RelativePath "patcher.yaml")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "Project patcher manifest was not found for '$($Feature.id)'. Expected features/$($Feature.id)/patcher.yml in $($Feature.path)."
}

function Get-ProjectPatcherConfig {
    param([Parameter(Mandatory = $true)] [object]$Feature)

    $path = Resolve-ProjectPatcherConfig -Feature $Feature
    $config = Read-YamlFile -Path $path
    if ($null -eq $config) {
        throw "Project patcher manifest is empty: $path"
    }
    return [pscustomobject]@{
        Path = $path
        Config = $config
    }
}

function Get-GeneratedFlag {
    param(
        [Parameter(Mandatory = $true)] [object]$PatcherConfig,
        [Parameter(Mandatory = $true)] [string]$Name,
        [bool]$Default = $false
    )

    $generated = Get-FeatureProperty -Object $PatcherConfig -Name "generated" -Default $null
    $value = Get-FeatureProperty -Object $generated -Name $Name -Default $Default
    return [System.Convert]::ToBoolean($value)
}

function Get-PatcherLabel {
    param(
        [Parameter(Mandatory = $true)] [object]$Install,
        [Parameter(Mandatory = $true)] [object]$PatcherConfig
    )

    return [string](Get-FeatureProperty -Object $PatcherConfig -Name "label" -Default $Install.label)
}

function Get-PatcherPatchVersion {
    param([Parameter(Mandatory = $true)] [object]$PatcherConfig)

    $value = [string](Get-FeatureProperty -Object $PatcherConfig -Name "patchVersion" -Default "")
    return $value.Trim()
}

function Test-PatcherPatchVersionAutoIncrement {
    param([Parameter(Mandatory = $true)] [object]$PatcherConfig)

    $value = Get-FeatureProperty -Object $PatcherConfig -Name "autoIncrementPatchVersion" -Default $false
    return [System.Convert]::ToBoolean($value)
}

function Get-IncrementedPatchVersion {
    param([Parameter(Mandatory = $true)] [string]$Version)

    $trimmed = $Version.Trim()
    if ([string]::IsNullOrWhiteSpace($trimmed)) {
        return "Build 1"
    }

    $match = [regex]::Match($trimmed, "^(?<prefix>.*?)(?<number>\d+)(?<suffix>\s*)$")
    if (-not $match.Success) {
        throw "Cannot auto-increment patchVersion '$Version'. Use a value ending in a number, for example 'Build 13'."
    }

    $number = [int64]$match.Groups["number"].Value
    return "$($match.Groups["prefix"].Value)$($number + 1)$($match.Groups["suffix"].Value)"
}

function Set-PatcherConfigPatchVersion {
    param(
        [Parameter(Mandatory = $true)] [object]$PatcherConfig,
        [Parameter(Mandatory = $true)] [string]$Version
    )

    if ($PatcherConfig.PSObject.Properties.Name -contains "patchVersion") {
        $PatcherConfig.patchVersion = $Version
    }
    else {
        $PatcherConfig | Add-Member -MemberType NoteProperty -Name "patchVersion" -Value $Version
    }
}

function Set-PatcherPatchVersionInFile {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Version
    )

    $text = [System.IO.File]::ReadAllText($Path)
    $line = "patchVersion: $(ConvertTo-YamlSingleQuoted $Version)"
    $patchVersionLine = [regex]::new("(?m)^\s*patchVersion\s*:.*$")
    if ($patchVersionLine.IsMatch($text)) {
        $text = $patchVersionLine.Replace(
            $text,
            [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $line },
            1
        )
    }
    else {
        $insertAfter = [regex]::Match($text, "(?m)^label\s*:.*$")
        if ($insertAfter.Success) {
            $insertAt = $insertAfter.Index + $insertAfter.Length
            $text = $text.Insert($insertAt, "`r`n$line")
        }
        else {
            $text = "$line`r`n$text"
        }
    }

    Write-Utf8NoBom -Path $Path -Value $text
}

function Get-PatcherConfigDestinations {
    param([Parameter(Mandatory = $true)] [object]$PatcherConfig)

    $destinations = New-Object System.Collections.Generic.List[string]
    foreach ($file in (Get-FeatureArray $PatcherConfig "files")) {
        $destination = Normalize-ClientDestination ([string](Get-FeatureProperty -Object $file -Name "destination" -Default ""))
        if (-not [string]::IsNullOrWhiteSpace($destination) -and -not $destinations.Contains($destination)) {
            [void]$destinations.Add($destination)
        }
    }

    return $destinations.ToArray()
}

function Get-ManagedClientDestinations {
    param(
        [Parameter(Mandatory = $true)] [object[]]$Deployments,
        [Parameter(Mandatory = $true)] [object]$InstallManifest
    )

    $managed = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($deployment in $Deployments) {
        foreach ($destination in (Get-PatcherConfigDestinations -PatcherConfig $deployment.PatcherConfig)) {
            [void]$managed.Add($destination)
        }
    }

    foreach ($install in (Get-FeatureArray $InstallManifest "installs")) {
        foreach ($destination in (Get-FeatureArray $install "forbiddenClientFiles")) {
            $normalized = Normalize-ClientDestination ([string]$destination)
            if (-not [string]::IsNullOrWhiteSpace($normalized)) {
                [void]$managed.Add($normalized)
            }
        }
    }

    return @($managed) | Sort-Object
}

function Test-ManagedEquiXmlNeeded {
    param([Parameter(Mandatory = $true)] [string[]]$ManagedDestinations)

    foreach ($destination in $ManagedDestinations) {
        if ($destination.StartsWith("uifiles/default/EQUI_", [System.StringComparison]::OrdinalIgnoreCase) -and
            $destination.EndsWith(".xml", [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Write-WorkspaceDeleteList {
    param(
        [Parameter(Mandatory = $true)] [string]$PayloadPath,
        [Parameter(Mandatory = $true)] [string[]]$ManagedDestinations,
        [Parameter(Mandatory = $true)] [AllowEmptyCollection()] [object[]]$CopiedFiles,
        [switch]$DryRun
    )

    $kept = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $CopiedFiles) {
        $destination = Normalize-ClientDestination ([string]$file.destination)
        if (-not [string]::IsNullOrWhiteSpace($destination)) {
            [void]$kept.Add($destination)
        }
    }

    $deletes = New-Object System.Collections.Generic.List[string]
    foreach ($destination in $ManagedDestinations) {
        $normalized = Normalize-ClientDestination $destination
        if ([string]::IsNullOrWhiteSpace($normalized) -or $kept.Contains($normalized)) {
            continue
        }

        if (-not $deletes.Contains($normalized)) {
            [void]$deletes.Add($normalized)
        }
    }

    if ($deletes.Count -eq 0) {
        return
    }

    $deletePath = Join-Path $PayloadPath "delete.txt"
    if ($DryRun) {
        Write-Host "Would write switch cleanup list: $deletePath"
        foreach ($delete in $deletes | Sort-Object) {
            Write-Host "  delete $delete"
        }
        return
    }

    Write-Utf8NoBom -Path $deletePath -Value (($deletes | Sort-Object) -join "`n")
}

function Add-UiIncludesToFile {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string[]]$UiIncludes
    )

    if ($UiIncludes.Count -eq 0) {
        return
    }

    $lines = New-Object System.Collections.Generic.List[string]
    Get-Content -LiteralPath $Path | ForEach-Object { [void]$lines.Add($_) }

    foreach ($windowXml in $UiIncludes) {
        $text = $lines -join "`n"
        $pattern = "<Include>\s*$([regex]::Escape($windowXml))\s*</Include>"
        if ($text -match $pattern) {
            continue
        }

        $insertIndex = -1
        for ($index = 0; $index -lt $lines.Count; $index++) {
            if ($lines[$index] -match "<Include>\s*EQUI_TopLevelScreens\.xml\s*</Include>") {
                $insertIndex = $index
                break
            }
        }
        if ($insertIndex -lt 0) {
            for ($index = 0; $index -lt $lines.Count; $index++) {
                if ($lines[$index] -match "</Composite>") {
                    $insertIndex = $index
                    break
                }
            }
        }
        if ($insertIndex -lt 0) {
            throw "Could not find a safe include insertion point in EQUI.xml: $Path"
        }

        $lines.Insert($insertIndex, "`t`t<Include>$windowXml</Include>")
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllLines($Path, $lines.ToArray(), $encoding)
}

function Get-UiIncludeNames {
    param([Parameter(Mandatory = $true)] [object]$PatcherConfig)

    $names = New-Object System.Collections.Generic.List[string]
    $generated = Get-FeatureProperty -Object $PatcherConfig -Name "generated" -Default $null
    foreach ($include in (Get-FeatureArray $generated "equiIncludes")) {
        if (-not [string]::IsNullOrWhiteSpace($include) -and -not $names.Contains($include)) {
            [void]$names.Add($include)
        }
    }

    $autoIncludeFiles = Get-GeneratedFlag -PatcherConfig $PatcherConfig -Name "equiAutoIncludeFiles" -Default $true
    if (-not $autoIncludeFiles) {
        return $names.ToArray()
    }

    foreach ($file in (Get-FeatureArray $PatcherConfig "files")) {
        $destinationRelative = (ConvertTo-NativeSlash $file.destination)
        $destinationName = Split-Path -Leaf $destinationRelative
        if ($destinationRelative.StartsWith("uifiles\default\", [System.StringComparison]::OrdinalIgnoreCase) -and
            $destinationName.StartsWith("EQUI_", [System.StringComparison]::OrdinalIgnoreCase) -and
            $destinationName.EndsWith(".xml", [System.StringComparison]::OrdinalIgnoreCase) -and
            -not $destinationName.Equals("EQUI.xml", [System.StringComparison]::OrdinalIgnoreCase) -and
            -not $names.Contains($destinationName)) {
            [void]$names.Add($destinationName)
        }
    }

    return $names.ToArray()
}

function Copy-WorkspacePayload {
    param(
        [Parameter(Mandatory = $true)] [object]$Install,
        [Parameter(Mandatory = $true)] [object]$InstallManifest,
        [Parameter(Mandatory = $true)] [object]$Feature,
        [Parameter(Mandatory = $true)] [object]$FeatureManifest,
        [Parameter(Mandatory = $true)] [object]$PatcherConfig,
        [Parameter(Mandatory = $true)] [string]$PayloadPath,
        [switch]$AllowMissingClientFiles,
        [switch]$SkipEQUIXml,
        [switch]$ForceEQUIXml,
        [switch]$DryRun
    )

    $copied = New-Object System.Collections.Generic.List[object]
    $missing = New-Object System.Collections.Generic.List[object]
    $uiIncludes = @(Get-UiIncludeNames -PatcherConfig $PatcherConfig)

    foreach ($file in (Get-FeatureArray $PatcherConfig "files")) {
        $sourceFeatureId = [string](Get-FeatureProperty -Object $file -Name "sourceFeature" -Default $Feature.id)
        $sourceFeature = if ($sourceFeatureId.Equals($Feature.id, [System.StringComparison]::OrdinalIgnoreCase)) {
            $Feature
        }
        else {
            Get-FeatureById -Manifest $FeatureManifest -Id $sourceFeatureId
        }
        $sourceRelative = [string](Get-FeatureProperty -Object $file -Name "source" -Default "")
        $destinationRelative = [string](Get-FeatureProperty -Object $file -Name "destination" -Default "")
        $optional = [System.Convert]::ToBoolean((Get-FeatureProperty -Object $file -Name "optional" -Default $false))
        if ([string]::IsNullOrWhiteSpace($sourceRelative) -or [string]::IsNullOrWhiteSpace($destinationRelative)) {
            throw "Invalid file entry in $($Feature.id) patcher manifest. Both source and destination are required."
        }

        $source = Join-NativePath -Root $sourceFeature.path -RelativePath $sourceRelative
        $destination = Join-NativePath -Root $PayloadPath -RelativePath $destinationRelative

        if (-not (Test-Path -LiteralPath $source)) {
            $item = [pscustomobject]@{
                sourceFeature = $sourceFeatureId
                source = $source
                destination = $destinationRelative
            }
            [void]$missing.Add($item)
            if (-not $AllowMissingClientFiles -and -not $optional) {
                throw "Client source file is missing for $($Install.id): $source"
            }
            Write-Host "Skipping missing client source file for $($Install.id): $source" -ForegroundColor Yellow
            continue
        }

        if ($DryRun) {
            Write-Host "Would stage: $source -> $destination"
        }
        else {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
            Copy-Item -LiteralPath $source -Destination $destination -Force
        }
        [void]$copied.Add([pscustomobject]@{
            sourceFeature = $sourceFeatureId
            source = $source
            destination = $destinationRelative
        })
    }

    if (Get-GeneratedFlag -PatcherConfig $PatcherConfig -Name "eqhost" -Default $true) {
        if (-not [string]::IsNullOrWhiteSpace($LoginHost)) {
            if ($LoginPort -gt 0) {
                $eqhostText = "[LoginServer]`r`nHost=${LoginHost}:${LoginPort}`r`n"
            }
            else {
                $eqhostText = "[LoginServer]`r`nHost=${LoginHost}`r`n"
            }
        }
        else {
            $eqhostText = New-ClientEqHostText -Install $Install -Manifest $InstallManifest
        }
        $eqhostPath = Join-Path $PayloadPath "eqhost.txt"
        if ($DryRun) {
            Write-Host "Would stage generated eqhost: $eqhostPath"
        }
        else {
            Write-Utf8NoBom -Path $eqhostPath -Value $eqhostText
        }
        [void]$copied.Add([pscustomobject]@{
            sourceFeature = "generated"
            source = "workspace login config"
            destination = "eqhost.txt"
        })
    }

    $generated = Get-FeatureProperty -Object $PatcherConfig -Name "generated" -Default $null
    $explicitEquiXml = Get-FeatureProperty -Object $generated -Name "equiXml" -Default $null
    $writeEquiXml = if ($null -eq $explicitEquiXml) {
        $uiIncludes.Count -gt 0
    }
    else {
        [System.Convert]::ToBoolean($explicitEquiXml)
    }
    if ($null -eq $explicitEquiXml -and $ForceEQUIXml) {
        $writeEquiXml = $true
    }
    if (-not $SkipEQUIXml -and $writeEquiXml -and ($uiIncludes.Count -gt 0 -or $ForceEQUIXml)) {
        $equiSource = Join-Path $InstallManifest.cleanClientSource "uifiles\default\EQUI.xml"
        if (-not (Test-Path -LiteralPath $equiSource)) {
            $equiSource = Join-Path $Install.clientPath "uifiles\default\EQUI.xml"
        }
        if (-not (Test-Path -LiteralPath $equiSource)) {
            throw "Cannot stage EQUI.xml for $($Install.id); no clean or target EQUI.xml was found."
        }

        $equiDestination = Join-Path $PayloadPath "uifiles\default\EQUI.xml"
        if ($DryRun) {
            Write-Host "Would stage EQUI.xml with includes: $($uiIncludes -join ', ')"
        }
        else {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $equiDestination) | Out-Null
            Copy-Item -LiteralPath $equiSource -Destination $equiDestination -Force
            if ($uiIncludes.Count -gt 0) {
                Add-UiIncludesToFile -Path $equiDestination -UiIncludes $uiIncludes
            }
        }
        [void]$copied.Add([pscustomobject]@{
            sourceFeature = "generated"
            source = $equiSource
            destination = "uifiles/default/EQUI.xml"
        })
    }

    return [pscustomobject]@{
        Copied = $copied.ToArray()
        Missing = $missing.ToArray()
        UiIncludes = $uiIncludes
    }
}

function Write-ProjectMetadata {
    param(
        [Parameter(Mandatory = $true)] [object]$Install,
        [Parameter(Mandatory = $true)] [object]$Feature,
        [Parameter(Mandatory = $true)] [object]$PatcherConfig,
        [Parameter(Mandatory = $true)] [string]$ConfigPath,
        [Parameter(Mandatory = $true)] [string]$PayloadPath,
        [Parameter(Mandatory = $true)] [AllowEmptyCollection()] [object[]]$CopiedFiles,
        [Parameter(Mandatory = $true)] [AllowEmptyCollection()] [object[]]$MissingFiles
    )

    $generated = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
    $label = Get-PatcherLabel -Install $Install -PatcherConfig $PatcherConfig
    $patchVersion = Get-PatcherPatchVersion -PatcherConfig $PatcherConfig
    $notes = New-Object System.Text.StringBuilder
    [void]$notes.AppendLine("$label Test Patch Feed")
    [void]$notes.AppendLine("")
    if (-not [string]::IsNullOrWhiteSpace($patchVersion)) {
        [void]$notes.AppendLine("Patch version: $patchVersion")
    }
    [void]$notes.AppendLine("Generated $generated from $($Feature.path).")
    [void]$notes.AppendLine("Patcher manifest: $ConfigPath")
    [void]$notes.AppendLine("")
    [void]$notes.AppendLine("Included files:")
    foreach ($file in $CopiedFiles | Sort-Object destination) {
        [void]$notes.AppendLine("- $($file.destination)")
    }
    if ($MissingFiles.Count -gt 0) {
        [void]$notes.AppendLine("")
        [void]$notes.AppendLine("Missing files skipped:")
        foreach ($file in $MissingFiles | Sort-Object destination) {
            [void]$notes.AppendLine("- $($file.destination) from $($file.source)")
        }
    }

    $statusValue = if ($MissingFiles.Count -gt 0) { "degraded" } else { "online" }
    $message = if ($MissingFiles.Count -gt 0) {
        "$label feed generated with missing optional test assets."
    }
    else {
        "$label feed is ready for client testing."
    }

    $status = @"
status: '$statusValue'
message: '$($message.Replace("'", "''"))'
environment: 'local-test'
generated: '$generated'
version: '$($patchVersion.Replace("'", "''"))'
"@

    Write-Utf8NoBom -Path (Join-Path $PayloadPath "patch_notes.txt") -Value $notes.ToString()
    Write-Utf8NoBom -Path (Join-Path $PayloadPath "patcher_status.yml") -Value $status
}

function Get-LauncherInputHash {
    param(
        [Parameter(Mandatory = $true)] [string]$BuildRoot,
        [Parameter(Mandatory = $true)] [string]$FeedBaseUrl,
        [Parameter(Mandatory = $true)] [string]$PatcherFileName,
        [Parameter(Mandatory = $true)] [string]$PatcherLabel
    )

    $root = [System.IO.Path]::GetFullPath($BuildRoot).TrimEnd("\", "/")
    $excludedDirectories = @(
        "\bin\",
        "\obj\",
        "\packages\",
        "\.vs\",
        "\Temp\"
    )
    $excludedFiles = @(
        "SolutionInfo.cs",
        "eqemupatcher.yml",
        "launcher-build-cache.json"
    )

    $signature = New-Object System.Text.StringBuilder
    [void]$signature.AppendLine("VERSION=0.1.0.0")
    [void]$signature.AppendLine("BUILD_CONFIGURATION=Release")
    [void]$signature.AppendLine("SERVER_NAME=$PatcherLabel")
    [void]$signature.AppendLine("FILE_NAME=$PatcherFileName")
    [void]$signature.AppendLine("PATCH_BASE_URL=$FeedBaseUrl")
    [void]$signature.AppendLine("FILELIST_URL=$FeedBaseUrl")
    [void]$signature.AppendLine("PATCHER_URL=$FeedBaseUrl")
    [void]$signature.AppendLine("PATCH_NOTES_URL=$($FeedBaseUrl)patch_notes.txt")
    [void]$signature.AppendLine("SERVICE_STATUS_URL=$($FeedBaseUrl)patcher_status.yml")

    $files = Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object {
        $relative = $_.FullName.Substring($root.Length).TrimStart("\", "/")
        $pathKey = "\" + $relative.Replace("/", "\")
        $include = $true
        foreach ($directory in $excludedDirectories) {
            if ($pathKey.IndexOf($directory, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
                $include = $false
                break
            }
        }

        $include -and ($excludedFiles -notcontains $_.Name)
    } | Sort-Object FullName

    foreach ($file in $files) {
        $relative = $file.FullName.Substring($root.Length).TrimStart("\", "/").Replace("\", "/")
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash
        [void]$signature.AppendLine("$relative|$($file.Length)|$hash")
    }

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($signature.ToString())
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString($sha.ComputeHash($bytes))).Replace("-", "")
    }
    finally {
        $sha.Dispose()
    }
}

function Invoke-PatcherBuild {
    param(
        [Parameter(Mandatory = $true)] [string]$BuildRoot,
        [Parameter(Mandatory = $true)] [object]$Install,
        [Parameter(Mandatory = $true)] [string]$FeedBaseUrl,
        [Parameter(Mandatory = $true)] [string]$PatcherFileName,
        [Parameter(Mandatory = $true)] [string]$PatcherLabel
    )

    $exePath = Join-Path $BuildRoot "EQEmu Patcher\bin\Release\$PatcherFileName.exe"
    $cachePath = Join-Path $BuildRoot "launcher-build-cache.json"
    $inputHash = Get-LauncherInputHash `
        -BuildRoot $BuildRoot `
        -FeedBaseUrl $FeedBaseUrl `
        -PatcherFileName $PatcherFileName `
        -PatcherLabel $PatcherLabel

    if ((Test-Path -LiteralPath $exePath) -and (Test-Path -LiteralPath $cachePath)) {
        try {
            $cache = Get-Content -LiteralPath $cachePath -Raw | ConvertFrom-Json
            if ($cache.inputHash -eq $inputHash -and $cache.patcherFileName -eq $PatcherFileName) {
                Write-Host "Launcher unchanged; reusing $exePath"
                return $exePath
            }
        }
        catch {
            Write-Host "Ignoring stale launcher build cache: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }

    $saved = @{
        VERSION = $env:VERSION
        BUILD_CONFIGURATION = $env:BUILD_CONFIGURATION
        BUILD_TARGET = $env:BUILD_TARGET
        MSBUILD_VERBOSITY = $env:MSBUILD_VERBOSITY
        SERVER_NAME = $env:SERVER_NAME
        FILE_NAME = $env:FILE_NAME
        PATCH_BASE_URL = $env:PATCH_BASE_URL
        FILELIST_URL = $env:FILELIST_URL
        PATCHER_URL = $env:PATCHER_URL
        PATCH_NOTES_URL = $env:PATCH_NOTES_URL
        SERVICE_STATUS_URL = $env:SERVICE_STATUS_URL
    }

    try {
        $env:VERSION = "0.1.0.0"
        $env:BUILD_CONFIGURATION = "Release"
        $env:BUILD_TARGET = "Rebuild"
        $env:MSBUILD_VERBOSITY = "minimal"
        $env:SERVER_NAME = $PatcherLabel
        $env:FILE_NAME = $PatcherFileName
        $env:PATCH_BASE_URL = $FeedBaseUrl
        $env:FILELIST_URL = $FeedBaseUrl
        $env:PATCHER_URL = $FeedBaseUrl
        $env:PATCH_NOTES_URL = "$($FeedBaseUrl)patch_notes.txt"
        $env:SERVICE_STATUS_URL = "$($FeedBaseUrl)patcher_status.yml"

        Push-Location -LiteralPath $BuildRoot
        try {
            $buildOutput = & cmd.exe /c build.bat 2>&1
            $exitCode = $LASTEXITCODE
            $buildOutput | ForEach-Object { Write-Host $_ }
            if ($exitCode -ne 0) {
                throw "Patcher build failed for $($Install.id) with exit code $exitCode."
            }
        }
        finally {
            Pop-Location
        }
    }
    finally {
        foreach ($key in $saved.Keys) {
            if ($null -eq $saved[$key]) {
                Remove-Item -Path "env:$key" -ErrorAction SilentlyContinue
            }
            else {
                Set-Item -Path "env:$key" -Value $saved[$key]
            }
        }
    }

    if (-not (Test-Path -LiteralPath $exePath)) {
        throw "Built patcher executable was not found: $exePath"
    }

    $cache = [pscustomobject]@{
        inputHash = $inputHash
        exeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exePath).Hash
        patcherFileName = $PatcherFileName
        feedBaseUrl = $FeedBaseUrl
        patcherLabel = $PatcherLabel
        builtUtc = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    }
    Write-Utf8NoBom -Path $cachePath -Value (($cache | ConvertTo-Json -Depth 4) + "`n")

    return $exePath
}

function Write-WorkspaceIndex {
    param(
        [Parameter(Mandatory = $true)] [string]$OutputPath,
        [Parameter(Mandatory = $true)] [object[]]$Results,
        [Parameter(Mandatory = $true)] [string]$Client
    )

    $rows = New-Object System.Text.StringBuilder
    foreach ($result in $Results | Sort-Object id) {
        $statusClass = if ($result.missingCount -gt 0) { "warn" } else { "ok" }
        [void]$rows.AppendLine("<tr>")
        [void]$rows.AppendLine("<td><a href=""$(ConvertTo-Html $result.id)/"">$(ConvertTo-Html $result.label)</a></td>")
        [void]$rows.AppendLine("<td>$(ConvertTo-Html $result.clientName)</td>")
        [void]$rows.AppendLine("<td><a href=""$(ConvertTo-Html $result.fileList)"">manifest</a></td>")
        [void]$rows.AppendLine("<td class=""$statusClass"">$($result.downloadCount) files</td>")
        [void]$rows.AppendLine("<td class=""$statusClass"">$($result.missingCount) missing</td>")
        [void]$rows.AppendLine("</tr>")
    }

    $html = @"
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>EQEmu Workspace Patch Feeds</title>
  <style>
    body { margin: 0; font-family: Segoe UI, Arial, sans-serif; background: #10151c; color: #e6edf5; }
    main { max-width: 1120px; margin: 42px auto; padding: 0 24px; }
    h1 { font-size: 32px; margin: 0 0 6px; }
    p { color: #aab8c8; }
    table { border-collapse: collapse; width: 100%; margin-top: 24px; background: #18202b; }
    th, td { border-bottom: 1px solid #2b3646; padding: 12px 14px; text-align: left; }
    th { color: #8fa3ba; font-weight: 600; }
    a { color: #e0b55c; text-decoration: none; }
    .ok { color: #72d391; }
    .warn { color: #e0b55c; }
  </style>
</head>
<body>
  <main>
    <h1>EQEmu Workspace Patch Feeds</h1>
    <p>Local test feeds generated for isolated EQEmu feature clients.</p>
    <table>
      <thead><tr><th>Project</th><th>Client</th><th>RoF Manifest</th><th>Downloads</th><th>Missing</th></tr></thead>
      <tbody>
$rows
      </tbody>
    </table>
  </main>
</body>
</html>
"@

    Write-Utf8NoBom -Path (Join-Path $OutputPath "index.html") -Value $html
}

function Write-WorkspaceManifest {
    param(
        [Parameter(Mandatory = $true)] [string]$OutputPath,
        [Parameter(Mandatory = $true)] [object[]]$Results,
        [Parameter(Mandatory = $true)] [string]$BaseUrl
    )

    $manifest = [pscustomobject]@{
        service = "eqemu-workspace-patchers"
        baseUrl = $BaseUrl
        generatedUtc = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
        feeds = $Results
    }

    $json = $manifest | ConvertTo-Json -Depth 8
    Write-Utf8NoBom -Path (Join-Path $OutputPath "workspace_patchers.json") -Value $json

    $yaml = New-Object System.Text.StringBuilder
    [void]$yaml.AppendLine("service: 'eqemu-workspace-patchers'")
    [void]$yaml.AppendLine("baseUrl: $(ConvertTo-YamlSingleQuoted $BaseUrl)")
    [void]$yaml.AppendLine("generatedUtc: $(ConvertTo-YamlSingleQuoted $manifest.generatedUtc)")
    [void]$yaml.AppendLine("feeds:")
    foreach ($result in $Results | Sort-Object id) {
        [void]$yaml.AppendLine("  - id: $(ConvertTo-YamlSingleQuoted $result.id)")
        [void]$yaml.AppendLine("    label: $(ConvertTo-YamlSingleQuoted $result.label)")
        [void]$yaml.AppendLine("    featureId: $(ConvertTo-YamlSingleQuoted $result.featureId)")
        [void]$yaml.AppendLine("    clientName: $(ConvertTo-YamlSingleQuoted $result.clientName)")
        [void]$yaml.AppendLine("    clientPath: $(ConvertTo-YamlSingleQuoted $result.clientPath)")
        [void]$yaml.AppendLine("    patchClient: $(ConvertTo-YamlSingleQuoted $result.patchClient)")
        [void]$yaml.AppendLine("    feedUrl: $(ConvertTo-YamlSingleQuoted $result.feedUrl)")
        [void]$yaml.AppendLine("    fileList: $(ConvertTo-YamlSingleQuoted $result.fileList)")
        [void]$yaml.AppendLine("    downloadCount: $($result.downloadCount)")
        [void]$yaml.AppendLine("    missingCount: $($result.missingCount)")
    }
    Write-Utf8NoBom -Path (Join-Path $OutputPath "workspace_patchers.yml") -Value $yaml.ToString()
}

function New-DeploymentDescriptors {
    param(
        [Parameter(Mandatory = $true)] [object[]]$Installs,
        [Parameter(Mandatory = $true)] [object]$FeatureManifest,
        [Parameter(Mandatory = $true)] [string]$Client,
        [Parameter(Mandatory = $true)] [string]$BaseUrl
    )

    $items = New-Object System.Collections.Generic.List[object]
    foreach ($install in $Installs) {
        $feature = Get-FeatureById -Manifest $FeatureManifest -Id $install.featureId
        $configInfo = Get-ProjectPatcherConfig -Feature $feature
        $patcherConfig = $configInfo.Config
        $projectClient = [string](Get-FeatureProperty -Object $patcherConfig -Name "client" -Default $Client)
        if ([string]::IsNullOrWhiteSpace($projectClient)) {
            $projectClient = $Client
        }
        $patcherLabel = Get-PatcherLabel -Install $install -PatcherConfig $patcherConfig
        $feedBaseUrl = "$BaseUrl$($install.id)/"
        [void]$items.Add([pscustomobject]@{
            Install = $install
            Feature = $feature
            ConfigInfo = $configInfo
            PatcherConfig = $patcherConfig
            ProjectClient = $projectClient
            PatcherLabel = $patcherLabel
            FeedBaseUrl = $feedBaseUrl
        })
    }

    return $items.ToArray()
}

function Merge-WorkspaceResults {
    param(
        [Parameter(Mandatory = $true)] [string]$OutputPath,
        [Parameter(Mandatory = $true)] [object[]]$Results
    )

    $byId = New-Object 'System.Collections.Generic.Dictionary[string,object]' ([System.StringComparer]::OrdinalIgnoreCase)
    $existingPath = Join-Path $OutputPath "workspace_patchers.json"
    if (Test-Path -LiteralPath $existingPath) {
        try {
            $existing = Get-Content -LiteralPath $existingPath -Raw | ConvertFrom-Json
            foreach ($feed in @($existing.feeds)) {
                if ($null -ne $feed -and -not [string]::IsNullOrWhiteSpace($feed.id)) {
                    $byId[$feed.id] = $feed
                }
            }
        }
        catch {
            Write-Host "Existing workspace manifest could not be merged: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }

    foreach ($result in $Results) {
        if ($null -ne $result -and -not [string]::IsNullOrWhiteSpace($result.id)) {
            $byId[$result.id] = $result
        }
    }

    return @($byId.Values)
}

$workspaceScripts = Join-Path $WorkspaceRoot "scripts\InstallWorkspace.ps1"
if (-not (Test-Path -LiteralPath $workspaceScripts)) {
    throw "Workspace scripts were not found: $workspaceScripts"
}
. $workspaceScripts

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $PSScriptRoot "publish"
}
if ([string]::IsNullOrWhiteSpace($PayloadRoot)) {
    $PayloadRoot = Join-Path $PSScriptRoot "workspace-payloads"
}
if (-not $BaseUrl.EndsWith("/")) {
    $BaseUrl += "/"
}

$installManifest = Get-InstallManifest -ManifestPath (Join-Path $WorkspaceRoot "installs.json")
$featureManifest = Get-FeatureManifest -ManifestPath (Join-Path $WorkspaceRoot "features.json")
$patcherBuildRoot = Join-Path $PSScriptRoot "..\EQEmu Patcher"
$releaseScript = Join-Path $PSScriptRoot "New-PatcherRelease.ps1"

if (-not (Test-Path -LiteralPath $patcherBuildRoot)) {
    throw "Patcher build root was not found: $patcherBuildRoot"
}
if (-not (Test-Path -LiteralPath $releaseScript)) {
    throw "Release script was not found: $releaseScript"
}

$outputFull = Get-FullPath $OutputPath
$payloadFull = Get-FullPath $PayloadRoot
if ($Clean) {
    if ((Split-Path -Leaf $outputFull) -ne "publish") {
        throw "Refusing to clean output path that is not named publish: $outputFull"
    }
    if ($Project.Count -gt 0) {
        if (-not $DryRun) {
            New-Item -ItemType Directory -Force -Path $outputFull, $payloadFull | Out-Null
        }
        if ($ProjectOnlyWorkspaceManifest) {
            Write-Host "Project-only clean requested; workspace feed list will include only selected project feeds." -ForegroundColor Yellow
        }
        else {
            Write-Host "Partial project clean requested; preserving existing workspace feed list and other project feeds." -ForegroundColor Yellow
        }
    }
    else {
        Reset-Directory -Path $outputFull -ExpectedRoot $PSScriptRoot -DryRun:$DryRun
        Reset-Directory -Path $payloadFull -ExpectedRoot $PSScriptRoot -DryRun:$DryRun
    }
}
elseif (-not $DryRun) {
    New-Item -ItemType Directory -Force -Path $outputFull, $payloadFull | Out-Null
}

$installs = if ($Project.Count -gt 0) {
    foreach ($id in $Project) {
        Get-InstallById -Manifest $installManifest -Id $id
    }
}
else {
    @($installManifest.installs)
}

$deployments = @(New-DeploymentDescriptors -Installs $installs -FeatureManifest $featureManifest -Client $Client -BaseUrl $BaseUrl)
$allDeployments = if ($Project.Count -gt 0) {
    @(New-DeploymentDescriptors -Installs @($installManifest.installs) -FeatureManifest $featureManifest -Client $Client -BaseUrl $BaseUrl)
}
else {
    $deployments
}

$managedClientDestinations = @(Get-ManagedClientDestinations -Deployments $allDeployments -InstallManifest $installManifest)
$forceManagedEquiXml = (Test-ManagedEquiXmlNeeded -ManagedDestinations $managedClientDestinations)

$launcherLabel = "EQEmu Project Switcher Test Patcher"
if ($ProjectOnlyWorkspaceManifest -and @($deployments).Count -eq 1) {
    $launcherLabel = "$($deployments[0].PatcherLabel) Patcher"
    if ($launcherLabel -notmatch '^EQEmu\b') {
        $launcherLabel = "EQEmu $launcherLabel"
    }
}

$workspacePatcherExe = ""
if (-not $SkipLauncherBuild) {
    if ($DryRun) {
        Write-Host "Would build launcher: $launcherLabel"
    }
    else {
        $workspacePatcherExe = Invoke-PatcherBuild `
            -BuildRoot $patcherBuildRoot `
            -Install ([pscustomobject]@{ id = "workspace" }) `
            -FeedBaseUrl $BaseUrl `
            -PatcherFileName $PatcherFileName `
            -PatcherLabel $launcherLabel
    }
}

$results = New-Object System.Collections.Generic.List[object]
foreach ($deployment in $deployments) {
    $install = $deployment.Install
    $feature = $deployment.Feature
    $configInfo = $deployment.ConfigInfo
    $patcherConfig = $deployment.PatcherConfig
    $projectClient = $deployment.ProjectClient
    $patcherLabel = $deployment.PatcherLabel
    $feedBaseUrl = $deployment.FeedBaseUrl
    $projectOutput = Join-Path $outputFull $install.id
    $projectPayload = Join-Path $payloadFull $install.id

    Write-Host ""
    Write-Host "== Workspace patch feed: $($install.id) ==" -ForegroundColor Cyan
    Write-Host "Manifest: $($configInfo.Path)"
    Write-Host "Client: $($install.clientPath)"
    Write-Host "Feed: $feedBaseUrl"

    if ((Test-PatcherPatchVersionAutoIncrement -PatcherConfig $patcherConfig) -and -not $NoAutoIncrementPatchVersion) {
        $currentPatchVersion = Get-PatcherPatchVersion -PatcherConfig $patcherConfig
        $nextPatchVersion = Get-IncrementedPatchVersion -Version $currentPatchVersion
        if ($DryRun) {
            Write-Host "Would bump patchVersion: '$currentPatchVersion' -> '$nextPatchVersion'"
        }
        else {
            Set-PatcherPatchVersionInFile -Path $configInfo.Path -Version $nextPatchVersion
            Set-PatcherConfigPatchVersion -PatcherConfig $patcherConfig -Version $nextPatchVersion
            Write-Host "Bumped patchVersion: '$currentPatchVersion' -> '$nextPatchVersion'"
        }
    }

    Reset-Directory -Path $projectPayload -ExpectedRoot $payloadFull -DryRun:$DryRun
    Reset-Directory -Path $projectOutput -ExpectedRoot $outputFull -DryRun:$DryRun

    $payload = Copy-WorkspacePayload `
        -Install $install `
        -InstallManifest $installManifest `
        -Feature $feature `
        -FeatureManifest $featureManifest `
        -PatcherConfig $patcherConfig `
        -PayloadPath $projectPayload `
        -AllowMissingClientFiles:$AllowMissingClientFiles `
        -SkipEQUIXml:$SkipEQUIXml `
        -ForceEQUIXml:$forceManagedEquiXml `
        -DryRun:$DryRun

    if (-not $DryRun) {
        Write-WorkspaceDeleteList `
            -PayloadPath $projectPayload `
            -ManagedDestinations $managedClientDestinations `
            -CopiedFiles $payload.Copied

        Write-ProjectMetadata `
            -Install $install `
            -Feature $feature `
            -PatcherConfig $patcherConfig `
            -ConfigPath $configInfo.Path `
            -PayloadPath $projectPayload `
            -CopiedFiles $payload.Copied `
            -MissingFiles $payload.Missing
    }

    $patcherExe = $workspacePatcherExe

    if (-not $DryRun) {
        $releaseArgs = @{
            Client = $projectClient
            PayloadPath = $projectPayload
            OutputPath = $projectOutput
            BaseUrl = $feedBaseUrl
            PatcherFileName = $PatcherFileName
        }
        $patchVersion = Get-PatcherPatchVersion -PatcherConfig $patcherConfig
        if (-not [string]::IsNullOrWhiteSpace($patchVersion)) {
            $releaseArgs.Version = $patchVersion
        }
        if (-not [string]::IsNullOrWhiteSpace($patcherExe)) {
            $releaseArgs.PatcherExe = $patcherExe
        }
        & $releaseScript @releaseArgs

        if (-not $SkipClientInstall -and -not [string]::IsNullOrWhiteSpace($patcherExe)) {
            if (-not (Test-Path -LiteralPath $install.clientPath)) {
                throw "Client path does not exist for launcher install: $($install.clientPath)"
            }
            $clientLauncher = Join-Path $install.clientPath "$PatcherFileName.exe"
            Copy-Item -LiteralPath $patcherExe -Destination $clientLauncher -Force
            Write-Host "Installed launcher: $clientLauncher"
        }
    }

    $fileListPath = Join-Path $projectOutput "$projectClient\filelist_$projectClient.yml"
    $downloadCount = 0
    if (Test-Path -LiteralPath $fileListPath) {
        $inDownloads = $false
        foreach ($line in (Get-Content -LiteralPath $fileListPath)) {
            if ($line -match "^downloads:\s*$") {
                $inDownloads = $true
                continue
            }
            if ($line -match "^\S" -and $line -notmatch "^downloads:\s*$") {
                $inDownloads = $false
            }
            if ($inDownloads -and $line -match "^\s+- name:") {
                $downloadCount++
            }
        }
    }

    [void]$results.Add([pscustomobject]@{
        id = $install.id
        label = $patcherLabel
        featureId = $install.featureId
        patcherManifest = $configInfo.Path
        clientName = $install.clientName
        clientPath = $install.clientPath
        patchClient = $projectClient
        feedUrl = $feedBaseUrl
        fileList = "$($install.id)/$projectClient/filelist_$projectClient.yml"
        downloadCount = $downloadCount
        missingCount = @($payload.Missing).Count
        missing = @($payload.Missing)
    })
}

if (-not $DryRun) {
    $manifestResults = if ($Project.Count -gt 0 -and -not $ProjectOnlyWorkspaceManifest) {
        @(Merge-WorkspaceResults -OutputPath $outputFull -Results $results.ToArray())
    }
    else {
        $results.ToArray()
    }
    Write-WorkspaceIndex -OutputPath $outputFull -Results $manifestResults -Client $Client
    Write-WorkspaceManifest -OutputPath $outputFull -Results $manifestResults -BaseUrl $BaseUrl
}

Write-Host ""
Write-Host "Workspace patcher deployment generated at $outputFull" -ForegroundColor Green
Write-Host "Base URL: $BaseUrl"
if (@($results | Where-Object { $_.missingCount -gt 0 }).Count -gt 0) {
    Write-Host "Some feeds were generated with missing client files. Re-run without -AllowMissingClientFiles to enforce strict completeness." -ForegroundColor Yellow
}
