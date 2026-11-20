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
    datas=[(os.path.join(agent_path, 'assets'), 'assets')],
    hiddenimports=winrt_modules + ['PIL.Image', 'PIL.ImageOps', 'keyring.backends.Windows'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=['tkinter'],
    noarchive=False,
)
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name='MeoDeskCompanion',
          debug=False, bootloader_ignore_signals=False, strip=False,
          upx=False, console=False)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False,
               name='MeoDeskCompanion')
