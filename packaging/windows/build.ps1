# Build on Windows only; PyInstaller is not a cross-compiler.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Push-Location $root
try {
  python -m pip install --upgrade pip
  python -m pip install -r .\windows_agent\requirements-gui.txt 'pyinstaller>=6,<7'
  python -m PyInstaller --noconfirm --clean --distpath .\dist --workpath .\build .\packaging\windows\MeoDeskCompanion.spec
  $archive = Join-Path $root 'MeoDeskCompanion-Windows-x64.zip'
  Compress-Archive -Path .\dist\MeoDeskCompanion -DestinationPath $archive -Force
  Get-FileHash $archive -Algorithm SHA256 | Format-List
  Write-Host 'Built:' $archive
} finally {
  Pop-Location
}
