"""Check the Git index before publishing source; never print local values."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    names = subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
    values = []
    header = ROOT/'firmware/src/app_config.h'
    if header.exists():
        values = [v.encode() for v in re.findall(r'(?:WIFI_SSID|WIFI_PASSWORD|MEO_API_KEY|MEO_AP_PASSWORD)\s*=\s*"([^"]*)"', header.read_text(encoding='utf-8-sig'))
                  if len(v)>=6 and not v.startswith('CHANGE_')]
    bad=[]
    for name in names:
        if not name: continue
        if name in ('firmware/src/app_config.h','windows_agent/agent_config.json') or name.startswith('baseline/') or name.endswith(('.bin','.elf','.exe','.zip','.map')):
            bad.append(name); continue
        data=subprocess.check_output(['git','show',':'+name],cwd=ROOT)
        if any(value in data for value in values): bad.append(name)
    if bad:
        print('Do not publish these indexed paths:'); print('\n'.join(bad)); return 1
    print('Git index checked: no local credential matches or private build artifacts.'); return 0

if __name__ == '__main__': raise SystemExit(main())
