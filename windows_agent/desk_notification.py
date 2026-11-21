"""Small notification contract. Hidden content is removed before transmission."""
import re

def notification_payload(text="New notification",hidden=True,source="companion",ttl=8,priority=1):
    if not isinstance(source,str) or not re.fullmatch(r"[a-z0-9_-]{1,20}",source):
        raise ValueError("Invalid notification source")
    if type(hidden) is not bool or type(ttl) is not int or not 1<=ttl<=30 or type(priority) is not int or not 0<=priority<=2:
        raise ValueError("Invalid notification options")
    if not hidden and (not isinstance(text,str) or not 1<=len(text)<=60 or any(not 32<=ord(c)<=126 for c in text)):
        raise ValueError("Nhãn TFT cần 1–60 ký tự ASCII in được.")
    return dict(source=source,text="New notification" if hidden else text,hidden=hidden,ttl_s=ttl,priority=priority)
