$scriptPath = $MyInvocation.MyCommand.Path

$parentFolder = Split-Path -Path $scriptPath -Parent
$grandparentFolder = Split-Path -Path $parentFolder -Parent

Start-Process -WindowStyle Hidden code $grandparentFolder
