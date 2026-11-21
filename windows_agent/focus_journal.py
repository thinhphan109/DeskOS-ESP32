"""Local journal of completions actually observed over a fresh connection."""
from __future__ import annotations
from datetime import datetime
from pathlib import Path
import sqlite3
import time


class FocusJournal:
    def __init__(self,path=":memory:"):
        if str(path) != ":memory:": Path(path).parent.mkdir(parents=True,exist_ok=True)
        self.db=sqlite3.connect(str(path),timeout=0.2)
        self.db.execute("CREATE TABLE IF NOT EXISTS sessions(device TEXT,boot TEXT,seq INTEGER,day TEXT,minutes INTEGER,PRIMARY KEY(device,boot,seq))")
        self.cursors={}

    def observe(self,device,state,now=None,day=None):
        now=time.monotonic() if now is None else now
        day=day or datetime.now().astimezone().date().isoformat()
        boot,seq=state.get("state_boot_id"),state.get("focus_completed_total")
        minutes=state.get("focus_last_completed_min")
        if (not isinstance(boot,str) or not 1<=len(boot)<=64 or type(seq) is not int or not 0<=seq<=0xFFFFFFFF
                or not isinstance(device,str) or len(device)>128): return False
        previous=self.cursors.get(device)
        self.cursors[device]=(boot,seq,now)
        if len(self.cursors)>8: self.cursors.pop(next(iter(self.cursors)))
        if (not previous or previous[0]!=boot or seq!=previous[1]+1 or not 0<=now-previous[2]<3.5
                or type(minutes) is not int or not 1<=minutes<=180): return False
        with self.db:
            cursor=self.db.execute("INSERT OR IGNORE INTO sessions VALUES(?,?,?,?,?)",(device,boot,seq,day,minutes))
        return cursor.rowcount==1

    def summary(self,day=None):
        day=day or datetime.now().astimezone().date().isoformat()
        count,minutes=self.db.execute("SELECT COUNT(*),COALESCE(SUM(minutes),0) FROM sessions WHERE day=?",(day,)).fetchone()
        return count,minutes

    def close(self): self.db.close()
