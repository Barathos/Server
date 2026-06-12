# Generates Small/Large size variants of the Advanced Loot window family.
#
# Reads the shipped EQUI_NativeAutoLootWnd.xml (the Medium master), clones the
# five advloot screens plus every piece they reference, renames each cloned
# element with the preset suffix (_S / _L), scales all geometry by the preset
# percent, and maps fonts. The DLL picks a variant by appending the suffix to
# the screen and child names (NativeUiSized plumbing - see the handoff doc).
#
# Usage:
#   .\generate-ui-scales.ps1                 # writes ui\generated\EQUI_NativeAutoLootWnd_scaled.xml (variants only, for review)
#   .\generate-ui-scales.ps1 -Merge          # writes ui\EQUI_NativeAutoLootWnd.xml with variants appended to the master
#
# Shared art (Templates, Animations, NormalDecal references) is NOT suffixed -
# template animation instances are only a conflict when two buttons using the
# same template exist at once, and only one size variant of a window exists at
# a time.

param(
    [string]$MasterPath = (Join-Path $PSScriptRoot '..\ui\EQUI_NativeAutoLootWnd.xml'),
    [string]$OutputPath = '',
    [switch]$Merge
)

$ErrorActionPreference = 'Stop'

$screens = @(
    'NativeAutoLootWnd',
    'NativeAutoLootMenuWnd',
    'NativeAutoLootRulesWnd',
    'NativeAutoLootSettingsWnd',
    'NativeAutoLootManageWnd'
)

# percent: geometry scale; font map: master font value -> preset font value
$presets = @(
    @{ suffix = '_S'; percent = 80;  fonts = @{ '1'='1'; '2'='1'; '3'='2'; '4'='3'; '5'='4'; '6'='5'; '7'='6' } },
    @{ suffix = '_L'; percent = 125; fonts = @{ '1'='2'; '2'='3'; '3'='4'; '4'='5'; '5'='6'; '6'='7'; '7'='7' } }
)

# element names whose integer text content is geometry to scale
$scaledElements = @(
    'X', 'Y', 'CX', 'CY', 'ListHeight', 'Width',
    'LeftAnchorOffset', 'TopAnchorOffset', 'RightAnchorOffset', 'BottomAnchorOffset',
    'MinHSize', 'MinVSize'
)

$MasterPath = (Resolve-Path $MasterPath).Path
if (-not $OutputPath) {
    if ($Merge) {
        $OutputPath = $MasterPath
    }
    else {
        $generatedDir = Join-Path (Split-Path $MasterPath) 'generated'
        if (-not (Test-Path $generatedDir)) {
            New-Item -ItemType Directory -Path $generatedDir | Out-Null
        }
        $OutputPath = Join-Path $generatedDir 'EQUI_NativeAutoLootWnd_scaled.xml'
    }
}

$doc = New-Object System.Xml.XmlDocument
$doc.PreserveWhitespace = $true
$doc.Load($MasterPath)
$root = $doc.DocumentElement

# Strip any previously generated variants (and their separator whitespace)
# so -Merge is idempotent.
foreach ($node in @($root.ChildNodes)) {
    if ($node.NodeType -ne 'Element') { continue }
    $item = $node.GetAttribute('item')
    if ($item -and ($item.EndsWith('_S') -or $item.EndsWith('_L'))) {
        $previous = $node.PreviousSibling
        if ($previous -and ($previous.NodeType -eq 'Whitespace' -or ($previous.NodeType -eq 'Text' -and $previous.Value.Trim() -eq ''))) {
            $root.RemoveChild($previous) | Out-Null
        }
        $root.RemoveChild($node) | Out-Null
    }
}

# Index every top-level element that has an item attribute.
$elementsByName = @{}
foreach ($node in @($root.ChildNodes)) {
    if ($node.NodeType -ne 'Element') { continue }
    $item = $node.GetAttribute('item')
    if ($item) {
        if ($elementsByName.ContainsKey($item)) {
            throw "Duplicate top-level item '$item' in master."
        }
        $elementsByName[$item] = $node
    }
}

# Resolve each screen's clone set: the screen + every piece it references.
$cloneSets = @{}
foreach ($screenName in $screens) {
    if (-not $elementsByName.ContainsKey($screenName)) {
        throw "Screen '$screenName' not found in master."
    }

    $set = New-Object System.Collections.Generic.List[string]
    $set.Add($screenName)
    $screenNode = $elementsByName[$screenName]
    foreach ($piece in $screenNode.SelectNodes('Pieces')) {
        $pieceName = $piece.InnerText.Trim()
        if (-not $elementsByName.ContainsKey($pieceName)) {
            throw "Screen '$screenName' references missing piece '$pieceName'."
        }
        if (-not $set.Contains($pieceName)) {
            $set.Add($pieceName)
        }
    }
    $cloneSets[$screenName] = $set
}

function Scale-Int([string]$text, [int]$percent) {
    $value = 0
    if (-not [int]::TryParse($text.Trim(), [ref]$value)) {
        return $null
    }
    # round half up to match the DLL's integer math: (v * pct + 50) / 100
    return [string][int][math]::Floor($value * $percent / 100.0 + 0.5)
}

function Process-Node([System.Xml.XmlElement]$element, $preset, $renameSet) {
    foreach ($child in @($element.ChildNodes)) {
        if ($child.NodeType -ne 'Element') { continue }

        if ($child.Name -eq 'ScreenID' -or $child.Name -eq 'Pieces') {
            $name = $child.InnerText.Trim()
            if ($renameSet.Contains($name)) {
                $child.InnerText = $name + $preset.suffix
            }
            continue
        }

        if ($child.Name -eq 'Font') {
            $mapped = $preset.fonts[$child.InnerText.Trim()]
            if ($mapped) {
                $child.InnerText = $mapped
            }
            continue
        }

        if ($scaledElements -contains $child.Name -and -not $child.HasChildNodes -or
            ($scaledElements -contains $child.Name -and $child.ChildNodes.Count -eq 1 -and $child.FirstChild.NodeType -eq 'Text')) {
            $scaled = Scale-Int $child.InnerText $preset.percent
            if ($null -ne $scaled) {
                $child.InnerText = $scaled
                continue
            }
        }

        # recurse for containers like Location/Size/DecalSize/Columns
        Process-Node $child $preset $renameSet
    }
}

$generated = New-Object System.Collections.Generic.List[System.Xml.XmlNode]
$generatedCount = 0
foreach ($preset in $presets) {
    foreach ($screenName in $screens) {
        $renameSet = $cloneSets[$screenName]
        foreach ($name in $renameSet) {
            $clone = $elementsByName[$name].CloneNode($true)
            $clone.SetAttribute('item', $name + $preset.suffix)
            Process-Node $clone $preset $renameSet
            $generated.Add($clone) | Out-Null
            ++$generatedCount
        }
    }
}

if ($Merge) {
    foreach ($clone in $generated) {
        $root.AppendChild($doc.CreateTextNode("`n`t")) | Out-Null
        $root.AppendChild($clone) | Out-Null
    }
    $root.AppendChild($doc.CreateTextNode("`n")) | Out-Null
    $doc.Save($OutputPath)
}
else {
    $outDoc = New-Object System.Xml.XmlDocument
    $outDoc.PreserveWhitespace = $true
    $outRoot = $outDoc.CreateElement('XML')
    $outDoc.AppendChild($outRoot) | Out-Null
    foreach ($clone in $generated) {
        $outRoot.AppendChild($outDoc.CreateTextNode("`n`t")) | Out-Null
        $outRoot.AppendChild($outDoc.ImportNode($clone, $true)) | Out-Null
    }
    $outRoot.AppendChild($outDoc.CreateTextNode("`n")) | Out-Null
    $outDoc.Save($OutputPath)
}

# validate the output parses
$check = New-Object System.Xml.XmlDocument
$check.Load($OutputPath)

Write-Host "Generated $generatedCount elements ($($presets.Count) presets x $($screens.Count) screens) -> $OutputPath"
