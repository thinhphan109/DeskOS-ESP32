#pragma once

#include <stdint.h>
#include <string.h>

// One outstanding remote media command at a time, bounded TTL, monotonic seq.
// Web -> ESP -> Windows Agent -> ACK. No queued commands replay after reconnect.
class CommandQueue {
 public:
  struct Command { uint32_t seq; const char *action; uint32_t createdMs; };

  bool submit(const char *action, uint32_t nowMs) {
    if (!action || (strcmp(action, "play_pause") && strcmp(action, "next") &&
                    strcmp(action, "previous"))) return false;
    if (pending(nowMs)) return false;  // caller reports busy, never overwrite unacked command.
    ++lastSeq_;
    if (lastSeq_ == 0) ++lastSeq_;
    active_ = {lastSeq_, action, nowMs};
    pending_ = true;
    acknowledged_ = false;
    return true;
  }

  bool pending(uint32_t nowMs) {
    if (pending_ && (uint32_t)(nowMs - active_.createdMs) >= 8000UL) pending_ = false;
    return pending_;
  }

  const Command *get(uint32_t nowMs) {
    return pending(nowMs) ? &active_ : nullptr;
  }

  bool ack(uint32_t seq, bool ok, uint32_t nowMs) {
    if (!pending(nowMs) || active_.seq != seq) return false;
    pending_ = false;
    acknowledged_ = true;
    lastResult_ = ok;
    return true;
  }

  uint32_t sequence() const { return lastSeq_; }
  bool acknowledged() const { return acknowledged_; }
  bool lastResult() const { return lastResult_; }

 private:
  Command active_{};
  uint32_t lastSeq_ = 0;
  bool pending_ = false;
  bool acknowledged_ = false;
  bool lastResult_ = false;
};
