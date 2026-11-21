"""Bounded PC samples with source isolation and explicit gaps."""
from collections import deque
import math
import time

FIELDS = ("cpu_percent", "gpu_percent", "ram_percent")


class TelemetryHistory:
    def __init__(self):
        self.samples = deque(maxlen=61)
        self.source = None

    def add(self, data, host="", now=None):
        now = time.monotonic() if now is None else now
        source = (host, bool(data.get("demo")))
        if source != self.source:
            self.samples.clear(); self.source = source
        if data.get("data_age_ms", 0) >= 2500: return
        values = []
        for key in FIELDS:
            value = data.get(key)
            values.append(float(value) if type(value) in (int,float) and math.isfinite(value) and 0 <= value <= 100 else None)
        sample = (now, tuple(values))
        if self.samples and int(self.samples[-1][0]) == int(now): self.samples[-1] = sample
        else: self.samples.append(sample)

    def visible(self, now=None):
        now = time.monotonic() if now is None else now
        return [sample for sample in self.samples if now-60 <= sample[0] <= now]

    def segments(self, field, now=None):
        result, line, previous = [], [], None
        for timestamp, values in self.visible(now):
            value = values[field]
            if value is None or (previous is not None and timestamp-previous>2.5):
                if line: result.append(line)
                line=[]
            if value is not None: line.append((timestamp,value))
            previous=timestamp
        if line: result.append(line)
        return result
