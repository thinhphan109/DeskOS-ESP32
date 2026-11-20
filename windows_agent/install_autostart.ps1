# Creates a per-user Windows Startup shortcut. No administrator privileges.
$ErrorActionPreference = 'Stop'
$script = Join-Path $PSScriptRoot 'tray_agent.py'
$config = Join-Path $PSScriptRoot 'agent_config.json'
if (-not (Test-Path $config)) {
    throw 'Copy agent_config.example.json to agent_config.json and edit host/key first.'
}
$pythonw = $null
# Prefer the exact Python selected by 'py -3', where requirements were installed.
try {
    $pythonPath = (& py -3 -c 'import sys; print(sys.executable)' 2>$null | Select-Object -First 1).Trim()
    if (Test-Path $pythonPath) {
        $candidate = Join-Path (Split-Path $pythonPath -Parent) 'pythonw.exe'
        if (Test-Path $candidate) { $pythonw = $candidate }
    }
} catch { }
if (-not $pythonw) {
    $command = Get-Command pythonw.exe -ErrorAction SilentlyContinue
    if ($command) { $pythonw = $command.Source }
}
if (-not $pythonw) {
    throw 'pythonw.exe not found. Install Python for Windows with GUI support.'
}
$startup = [Environment]::GetFolderPath('Startup')
$shortcutPath = Join-Path $startup 'MeoDesk PC Monitor.lnk'
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $pythonw
$shortcut.Arguments = '"' + $script + '" --config "' + $config + '"'
$shortcut.WorkingDirectory = $PSScriptRoot
$shortcut.Description = 'MeoDesk local Windows telemetry agent'
$shortcut.Save()
Write-Host "Installed current-user Startup shortcut: $shortcutPath"
Write-Host 'It starts on next Windows login. To start now, run start_tray.bat.'
