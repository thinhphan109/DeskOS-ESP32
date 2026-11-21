"""Embed the standalone authoring CSS; firmware keeps a single self-contained UI."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
target=root/'src/web_ui.h'
source=target.read_text(encoding='utf-8')
start=source.index('<style>')+len('<style>'); end=source.index('</style>',start)
css=(root/'tools/mimi_studio.css').read_text(encoding='utf-8')
target.write_text(source[:start]+'\n'+css+'\n  '+source[end:],encoding='utf-8')
