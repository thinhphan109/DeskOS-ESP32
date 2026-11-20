# Removes ONLY the shortcut created by install_autostart.ps1.
$shortcut = Join-Path ([Environment]::GetFolderPath('Startup')) 'MeoDesk PC Monitor.lnk'
if (Test-Path $shortcut) {
    Remove-Item -LiteralPath $shortcut
    Write-Host 'MèoDesk Startup shortcut removed (agent and configuration preserved).'
} else {
    Write-Host 'No MèoDesk Startup shortcut was installed.'
}
