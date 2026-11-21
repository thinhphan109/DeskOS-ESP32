# Per-user install/upgrade. Device configuration stays in LocalAppData\MeoDesk.
param([switch]$EnableStartup, [switch]$NoShortcut)
$ErrorActionPreference = 'Stop'
$source = $PSScriptRoot
$destination = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Programs\mimi.desk'))
$workspace = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Programs'))
if (-not $destination.StartsWith($workspace + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
  throw 'Unexpected installation directory'
}
if (-not (Test-Path -LiteralPath (Join-Path $source 'mimi.desk.exe'))) { throw 'Extract the complete ZIP before installing' }
if ($source.TrimEnd('\') -eq $destination.TrimEnd('\')) { throw 'Run install.ps1 from the extracted release folder' }
$running = Get-Process -Name 'mimi.desk', 'MeoDeskCompanion' -ErrorAction SilentlyContinue
if ($running) { throw 'Exit mimi.desk from the system tray before installing or upgrading' }
New-Item -ItemType Directory -Path $destination -Force | Out-Null
# Copy file-by-file: never remove settings or recursively delete the previous install.
Get-ChildItem -LiteralPath $source -Force | Copy-Item -Destination $destination -Recurse -Force
$executable = Join-Path $destination 'mimi.desk.exe'
if (-not $NoShortcut) {
  $shortcutPath = Join-Path ([Environment]::GetFolderPath('Programs')) 'mimi.desk.lnk'
  $shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut($shortcutPath)
  $shortcut.TargetPath = $executable
  $shortcut.WorkingDirectory = $destination
  $shortcut.Save()
}
$registryPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$oldStartup = Get-ItemPropertyValue -LiteralPath $registryPath -Name 'MeoDeskCompanion' -ErrorAction SilentlyContinue
if ($EnableStartup -or $oldStartup) {
  New-Item -Path $registryPath -Force | Out-Null
  Set-ItemProperty -LiteralPath $registryPath -Name 'MeoDeskCompanion' -Value ('"' + $executable + '" --minimized')
}
Write-Host 'Installed:' $executable
Write-Host 'Device settings and pairing credentials were preserved. Startup changes only when requested or already enabled.'
