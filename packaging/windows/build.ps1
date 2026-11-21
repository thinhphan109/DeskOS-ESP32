# Build in an isolated environment so optional desktop tools do not inflate the EXE.
param([string]$Python = 'python', [switch]$SkipDependencies)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Push-Location $root
try {
  $packageEnvironment = Join-Path $root 'build\package-venv'
  $packagePython = Join-Path $packageEnvironment 'Scripts\python.exe'
  if (-not (Test-Path -LiteralPath $packagePython)) {
    & $Python -m venv $packageEnvironment
    if ($LASTEXITCODE -ne 0) { throw 'Cannot create packaging environment' }
  }
  if (-not $SkipDependencies) {
    & $packagePython -m pip install -r .\windows_agent\requirements-gui.txt 'pyinstaller>=6,<7'
    if ($LASTEXITCODE -ne 0) { throw 'Cannot install packaging dependencies' }
  }
  & $packagePython .\packaging\windows\prepare_metadata.py
  if ($LASTEXITCODE -ne 0) { throw 'Cannot generate executable metadata' }
  & $packagePython -m PyInstaller --noconfirm --clean --distpath .\dist --workpath .\build\pyinstaller .\packaging\windows\MeoDeskCompanion.spec
  if ($LASTEXITCODE -ne 0) { throw 'PyInstaller failed' }
  $output = Join-Path $root 'dist\mimi.desk'
  $reportPath = Join-Path $root 'build\packaged-self-check.json'
  $checker = Start-Process -FilePath (Join-Path $output 'mimi.desk.exe') -ArgumentList @('--self-check', ('"' + $reportPath + '"')) -WindowStyle Hidden -PassThru -Wait
  if ($checker.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $reportPath)) { throw 'Packaged dependency self-check failed' }
  Copy-Item -LiteralPath .\packaging\windows\install.ps1, .\packaging\windows\uninstall.ps1, .\packaging\windows\README-WINDOWS.txt -Destination $output
  & $packagePython -m pip freeze | Set-Content -LiteralPath (Join-Path $output 'dependencies.txt') -Encoding utf8
  $version = (& $packagePython -c "import sys;sys.path.insert(0,'windows_agent');from app_version import VERSION;print(VERSION)").Trim()
  $archive = Join-Path $root "dist\mimi.desk-$version-Windows-x64.zip"
  Compress-Archive -Path $output -DestinationPath $archive -Force
  Get-FileHash $archive -Algorithm SHA256 | Format-List
  Write-Host 'Built:' $archive
} finally {
  Pop-Location
}
