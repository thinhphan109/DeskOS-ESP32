# -*- mode: python ; coding: utf-8 -*-
import os
from PyInstaller.utils.hooks import collect_submodules

project_root = os.path.normpath(os.path.join(SPECPATH, '..', '..'))
agent_path = os.path.join(project_root, 'windows_agent')

winrt_modules = []
try:
    winrt_modules = collect_submodules('winrt')
except Exception:
    pass

a = Analysis(
    [os.path.join(agent_path, 'companion.py')],
    pathex=[agent_path],
    binaries=[],
    datas=[(os.path.join(agent_path, 'assets'), 'assets'),
           (os.path.join(project_root, 'build', 'windows-metadata', 'build-info.json'), '.')],
    hiddenimports=winrt_modules + ['PIL.Image', 'PIL.ImageOps', 'keyring.backends.Windows'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=['tkinter'],
    noarchive=False,
)
# Qt 6.12 uses Windows' system ICU. A same-named ICU from Strawberry/other
# developer tools on PATH can be collected instead and exports incompatible
# suffixed functions. Leave Windows system DLLs to the OS loader.
a.binaries = [entry for entry in a.binaries
              if os.path.basename(entry[0]).lower() not in ('icuuc.dll', 'icuin.dll', 'icudt.dll')]
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name='mimi.desk',
          icon=os.path.join(project_root,'build','windows-metadata','mimi.ico'),
          version=os.path.join(project_root,'build','windows-metadata','version.txt'),
          debug=False, bootloader_ignore_signals=False, strip=False,
          upx=False, console=False)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False,
               name='mimi.desk')
