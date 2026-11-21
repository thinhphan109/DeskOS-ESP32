"""Derive executable metadata and ICO from the existing project assets."""
import json
from pathlib import Path
import sys
from PIL import Image

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'windows_agent'))
from app_version import VERSION
output=root/'build/windows-metadata';output.mkdir(parents=True,exist_ok=True)
numbers=tuple(int(v) for v in VERSION.split('-')[0].split('.'))+(0,)
strings={"CompanyName":"mimi.desk","FileDescription":"mimi.desk Windows Companion",
         "FileVersion":VERSION,"InternalName":"mimi.desk","OriginalFilename":"mimi.desk.exe",
         "ProductName":"mimi.desk","ProductVersion":VERSION}
entries=",\n".join(f"StringStruct({key!r}, {value!r})" for key,value in strings.items())
metadata=f"""VSVersionInfo(ffi=FixedFileInfo(filevers={numbers!r},prodvers={numbers!r},mask=0x3f,flags=0x2,OS=0x40004,fileType=0x1,subtype=0x0,date=(0,0)),kids=[StringFileInfo([StringTable('040904B0',[{entries}])]),VarFileInfo([VarStruct('Translation',[1033,1200])])])"""
(output/'version.txt').write_text(metadata,encoding='utf8')
with Image.open(root/'windows_agent/assets/healthy.png') as image:
    image.convert('RGBA').save(output/'mimi.ico',format='ICO',sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
(output/'build-info.json').write_text(json.dumps({'version':VERSION,'configuration_included':False},indent=2),encoding='utf8')
print('Prepared Windows icon and version:',VERSION)
