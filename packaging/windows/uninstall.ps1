# Unregister app integration, preserving both settings and recoverable application files.
$ErrorActionPreference = 'Stop'
$registryPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$current = Get-ItemPropertyValue -LiteralPath $registryPath -Name 'MeoDeskCompanion' -ErrorAction SilentlyContinue
if ($current -and ($current -match 'mimi\.desk\.exe|MeoDeskCompanion\.exe')) {
  Remove-ItemProperty -LiteralPath $registryPath -Name 'MeoDeskCompanion'
}
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'mimi.desk.lnk'
if (Test-Path -LiteralPath $shortcut) { Remove-Item -LiteralPath $shortcut }
Write-Host 'Removed Start-menu/startup integration. Exit the app, then remove its installation folder if no longer needed.'
Write-Host 'Device settings and pairing credentials remain available for a later reinstall.'
