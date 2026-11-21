"""Explicit opt-in, per-user Windows startup entry. No elevated privileges."""
from pathlib import Path
import subprocess
import sys

KEY=r"Software\Microsoft\Windows\CurrentVersion\Run"
NAME="MeoDeskCompanion"


def launch_command(executable=None,script=None,frozen=None):
    executable=executable or sys.executable
    frozen=getattr(sys,"frozen",False) if frozen is None else frozen
    args=[str(Path(executable).resolve())]
    if not frozen: args.append(str(Path(script or Path(__file__).with_name("companion.py")).resolve()))
    return subprocess.list2cmdline(args+["--minimized"])


def is_enabled():
    if sys.platform!="win32": return False
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,KEY) as key:
            return winreg.QueryValueEx(key,NAME)[0]==launch_command()
    except OSError: return False


def set_enabled(enabled):
    if sys.platform!="win32": raise OSError("Windows startup is unavailable on this platform")
    import winreg
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER,KEY) as key:
        if enabled: winreg.SetValueEx(key,NAME,0,winreg.REG_SZ,launch_command())
        else:
            try: winreg.DeleteValue(key,NAME)
            except FileNotFoundError: pass
