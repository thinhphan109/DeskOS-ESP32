"""Embed the authored update UI script into the self-contained firmware page."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
target=root/'src/web_ui.h'
source=(root/'tools/update_studio.js').read_text(encoding='utf8')
text=target.read_text(encoding='utf8')
start='      // BEGIN MIMI UPDATE SCRIPT\n';end='      // END MIMI UPDATE SCRIPT\n'
block=start+'\n'.join('      '+line for line in source.splitlines())+'\n'+end
if start in text:
 a=text.index(start);b=text.index(end,a)+len(end);text=text[:a]+block+text[b:]
else:
 anchor='      function receiveState(data){'
 if anchor not in text:raise SystemExit('Update script insertion anchor not found')
 text=text.replace(anchor,block+anchor,1)
target.write_text(text,encoding='utf8')
