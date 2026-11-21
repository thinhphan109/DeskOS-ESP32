"""Bounded retry timing, shared by the network worker and desktop transport."""


class RetryBackoff:
    def __init__(self, initial=1.5, maximum=30.0):
        self.initial = initial
        self.maximum = maximum
        self.next_at = 0.0
        self.delay = initial

    def ready(self, now):
        return now >= self.next_at

    def failed(self, now):
        delay = self.delay
        self.next_at = now + delay
        self.delay = min(self.maximum, delay * 2)
        return delay

    def reset(self):
        self.next_at = 0.0
        self.delay = self.initial
