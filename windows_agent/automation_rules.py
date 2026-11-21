"""Portable bounded automation rule contract."""
WHENS=("cpu_above","gpu_above","hour","focus_complete","idle_above","music_playing")
ACTIONS=("scene","theme","celebrate","notify")
PRESET={"schema":1,"rules":[
    dict(enabled=True,when="cpu_above",threshold=90,hold_s=30,cooldown_s=300,do="notify",value=0,label="CPU busy"),
    dict(enabled=True,when="hour",threshold=23,hold_s=0,cooldown_s=3600,do="theme",value=1,label="Night desk")]}


def validate_rules(data):
    if not isinstance(data,dict) or type(data.get("schema")) is not int or data["schema"]!=1 or not isinstance(data.get("rules"),list) or len(data["rules"])>6:
        raise ValueError("Rules require schema 1 and at most six entries")
    clean=[]
    for rule in data["rules"]:
        if not isinstance(rule,dict) or set(rule)!={"enabled","when","threshold","hold_s","cooldown_s","do","value","label"}:
            raise ValueError("Invalid rule fields")
        if type(rule["enabled"]) is not bool or rule["when"] not in WHENS or rule["do"] not in ACTIONS:
            raise ValueError("Invalid condition or action")
        for key,low,high in (("threshold",0,7200),("hold_s",0,600),("cooldown_s",5,86400),("value",0,14)):
            if type(rule[key]) is not int or not low<=rule[key]<=high:raise ValueError(f"Invalid {key}")
        trigger,threshold=rule["when"],rule["threshold"]
        if ((trigger in ("cpu_above","gpu_above") and not 1<=threshold<=100) or (trigger=="hour" and threshold>23)
                or (trigger=="idle_above" and threshold<1) or (trigger in ("focus_complete","music_playing") and threshold!=0)
                or (trigger=="focus_complete" and rule["hold_s"]!=0)):
            raise ValueError("Invalid threshold or hold time for condition")
        if (rule["do"]=="theme" and rule["value"]>5) or (rule["do"] in ("notify","celebrate") and rule["value"]!=0):
            raise ValueError("Invalid action value")
        label=rule["label"]
        if not isinstance(label,str) or not 1<=len(label)<=32 or any(not 32<=ord(c)<=126 for c in label):
            raise ValueError("Rule label requires 1–32 printable ASCII characters")
        clean.append(dict(rule))
    return dict(schema=1,rules=clean)
