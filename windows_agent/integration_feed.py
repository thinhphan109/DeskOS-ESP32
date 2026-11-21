"""Typed feed payloads for three expiring external-data widgets."""
def validate_feed(data):
    if not isinstance(data,dict) or set(data)!={'name','value','unit','ttl_s'}:
        raise ValueError("Feed requires name, value, unit and ttl_s")
    for key,low,high in (('name',1,16),('value',1,20),('unit',0,8)):
        text=data[key]
        if not isinstance(text,str) or not low<=len(text)<=high or any(not 32<=ord(c)<=126 for c in text):
            raise ValueError(f"{key} must contain {low}..{high} printable ASCII characters")
    if type(data['ttl_s']) is not int or not 1<=data['ttl_s']<=300:
        raise ValueError("Feed TTL must be 1..300 seconds")
    return dict(data)
