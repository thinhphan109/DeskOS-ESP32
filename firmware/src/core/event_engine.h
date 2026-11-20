#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Fixed-size domain event timeline; no dynamic allocations or framework APIs.
// Events can expire from the live overlay without disappearing from history.
class EventEngine {
 public:
  enum Priority : uint8_t { INFO = 0, NOTICE = 1, WARNING = 2, CRITICAL = 3 };
  struct Event {
    uint32_t seq = 0;
    uint32_t createdMs = 0;
    uint32_t ttlMs = 0;
    Priority priority = INFO;
    char code[25] = "";
    char message[69] = "";
  };

  static constexpr uint8_t CAPACITY = 10;

  bool publish(const char *code, const char *message, Priority priority,
               uint32_t nowMs, uint32_t ttlMs = 8500) {
    if (!code || !*code || !message || !*message) return false;
    const Event *last = recentAt(0);
    if (last && strcmp(last->code, code) == 0 &&
        (uint32_t)(nowMs - last->createdMs) < 1500) return false;
    Event &event = buffer_[head_];
    ++lastSeq_;
    if (!lastSeq_) ++lastSeq_;
    event.seq = lastSeq_;
    event.createdMs = nowMs;
    event.ttlMs = ttlMs > 60000 ? 60000 : ttlMs;
    event.priority = priority;
    snprintf(event.code, sizeof(event.code), "%s", code);
    snprintf(event.message, sizeof(event.message), "%s", message);
    head_ = (head_ + 1) % CAPACITY;
    if (count_ < CAPACITY) ++count_;
    return true;
  }

  uint8_t count() const { return count_; }
  uint32_t latestSequence() const { return lastSeq_; }

  const Event *recentAt(uint8_t newestIndex) const {
    if (newestIndex >= count_) return nullptr;
    return &buffer_[(head_ + CAPACITY - 1 - newestIndex) % CAPACITY];
  }

  const Event *active(uint32_t nowMs) const {
    const Event *winner = nullptr;
    for (uint8_t i = 0; i < count_; ++i) {
      const Event *entry = recentAt(i);
      if (!entry || (uint32_t)(nowMs - entry->createdMs) >= entry->ttlMs) continue;
      if (!winner || entry->priority > winner->priority ||
          (entry->priority == winner->priority && entry->seq > winner->seq)) winner = entry;
    }
    return winner;
  }

 private:
  Event buffer_[CAPACITY]{};
  uint8_t head_ = 0;
  uint8_t count_ = 0;
  uint32_t lastSeq_ = 0;
};
