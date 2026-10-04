#
# Renders the icon sources of verselens/res/icon into the icon of the executable and the assets of the MSIX
# package. Every size takes the cut that is drawn for it, the small cuts drop the details that turn into mud.
# Needs rsvg-convert, the MSYS2 package mingw-w64-ucrt-x86_64-librsvg holds it.
#
# Examples:
#   tools/make_icons_verselens.ps1
#
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$iconDir = Join-Path $repoRoot "verselens/res/icon"
$assetsDir = Join-Path $repoRoot "verselens/res/msix/assets"
$icoPath = Join-Path $repoRoot "verselens/res/icon.ico"

# The tray takes the first image of the icon file as it is, see libs_external/traypp, so 32x32 comes first.
# Windows scales that one into the notification area, the other sizes serve explorer, taskbar and alt-tab.
$icoSizes = @(32, 16, 20, 24, 48, 64, 128, 256)

#
# Locate rsvg-convert, the MSYS2 shell the readme builds in holds it.
#
$rsvg = (Get-Command rsvg-convert -ErrorAction SilentlyContinue)?.Source
if($null -eq $rsvg)
{
  $rsvg = "C:/msys64/ucrt64/bin/rsvg-convert.exe"
}
if(-not (Test-Path $rsvg))
{
  throw "rsvg-convert not found. Install it with 'pacman -S mingw-w64-ucrt-x86_64-librsvg' or add it to the PATH."
}

function Get-IconSource([int] $size)
{
  if($size -le 24) { return Join-Path $iconDir "icon_small.svg" }
  if($size -le 48) { return Join-Path $iconDir "icon_medium.svg" }
  return Join-Path $iconDir "icon_full.svg"
}

function Convert-Svg([int] $size, [string] $out)
{
  & $rsvg -w $size -h $size (Get-IconSource $size) -o $out
  if($LASTEXITCODE -ne 0)
  {
    throw "rsvg-convert failed for size $size"
  }
}

#
# Converts a rendered png into the DIB an icon directory entry holds: a header of double height, the colors
# bottom up and the and-mask windows ignores as long as the colors carry an alpha channel.
#
function ConvertTo-IconImage([string] $png)
{
  $bitmap = [System.Drawing.Bitmap]::new($png)
  try
  {
    $width = $bitmap.Width
    $height = $bitmap.Height
    $rectangle = [System.Drawing.Rectangle]::new(0, 0, $width, $height)
    $data = $bitmap.LockBits($rectangle, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
      [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = $data.Stride
    $pixels = [byte[]]::new($stride * $height)
    try
    {
      [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $pixels, 0, $pixels.Length)
    }
    finally
    {
      $bitmap.UnlockBits($data)
    }

    $maskStride = [int][Math]::Floor(($width + 31) / 32) * 4
    $stream = [System.IO.MemoryStream]::new()
    $writer = [System.IO.BinaryWriter]::new($stream)
    $writer.Write([uint32] 40)
    $writer.Write([int32] $width)
    $writer.Write([int32] ($height * 2))
    $writer.Write([uint16] 1)
    $writer.Write([uint16] 32)
    $writer.Write([uint32] 0)
    $writer.Write([uint32] ($width * $height * 4 + $maskStride * $height))
    $writer.Write([int32] 0)
    $writer.Write([int32] 0)
    $writer.Write([uint32] 0)
    $writer.Write([uint32] 0)
    for($y = $height - 1; $y -ge 0; $y--)
    {
      $writer.Write($pixels, $y * $stride, $width * 4)
    }
    $writer.Write([byte[]]::new($maskStride * $height))
    $writer.Flush()
    # The comma keeps the pipeline from unrolling the array into single bytes.
    return , $stream.ToArray()
  }
  finally
  {
    $bitmap.Dispose()
  }
}

#
# Draws a rendered png centered on a transparent canvas, the wide tile of the store is the only asset that is
# no square.
#
function Save-Centered([string] $png, [int] $width, [int] $height, [string] $out)
{
  $source = [System.Drawing.Bitmap]::new($png)
  $canvas = [System.Drawing.Bitmap]::new($width, $height)
  try
  {
    $graphics = [System.Drawing.Graphics]::FromImage($canvas)
    try
    {
      $graphics.DrawImage($source, [int](($width - $source.Width) / 2), [int](($height - $source.Height) / 2),
        $source.Width, $source.Height)
    }
    finally
    {
      $graphics.Dispose()
    }
    $canvas.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
  }
  finally
  {
    $source.Dispose()
    $canvas.Dispose()
  }
}

$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "verselens_icons_$([System.IO.Path]::GetRandomFileName())"
try
{
  New-Item -ItemType Directory -Force $tempDir | Out-Null

  #
  # The icon of the executable, one entry per size.
  #
  $images = [System.Collections.Generic.List[byte[]]]::new()
  foreach($size in $icoSizes)
  {
    $png = Join-Path $tempDir "icon_$size.png"
    Convert-Svg $size $png
    $images.Add((ConvertTo-IconImage $png))
  }

  $stream = [System.IO.MemoryStream]::new()
  $writer = [System.IO.BinaryWriter]::new($stream)
  $writer.Write([uint16] 0)
  $writer.Write([uint16] 1)
  $writer.Write([uint16] $images.Count)
  $offset = 6 + 16 * $images.Count
  for($i = 0; $i -lt $images.Count; $i++)
  {
    # A size of 256 is stored as zero, the byte of the directory entry holds no larger value.
    $size = $icoSizes[$i]
    $writer.Write([byte] ($size -eq 256 ? 0 : $size))
    $writer.Write([byte] ($size -eq 256 ? 0 : $size))
    $writer.Write([byte] 0)
    $writer.Write([byte] 0)
    $writer.Write([uint16] 1)
    $writer.Write([uint16] 32)
    $writer.Write([uint32] $images[$i].Length)
    $writer.Write([uint32] $offset)
    $offset += $images[$i].Length
  }
  $images | ForEach-Object { $writer.Write($_) }
  $writer.Flush()
  [System.IO.File]::WriteAllBytes($icoPath, $stream.ToArray())
  Write-Host "wrote $icoPath, sizes $($icoSizes -join ', ')"

  #
  # The assets of the MSIX package, the manifest names them by size.
  #
  Convert-Svg 44 (Join-Path $assetsDir "Square44x44Logo.png")
  Convert-Svg 50 (Join-Path $assetsDir "StoreLogo.png")
  Convert-Svg 150 (Join-Path $assetsDir "Square150x150Logo.png")

  $wideLogo = Join-Path $tempDir "wide.png"
  Convert-Svg 150 $wideLogo
  Save-Centered $wideLogo 310 150 (Join-Path $assetsDir "Wide310x150Logo.png")
  Write-Host "wrote the assets in $assetsDir"
}
finally
{
  Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}
