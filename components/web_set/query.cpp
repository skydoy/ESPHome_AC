// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "query.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace esphome {
namespace web_set {

static constexpr float ROOM_MIN = -30.0f;
static constexpr float ROOM_MAX = 60.0f;
static constexpr float GRID_MIN = -100000.0f;
static constexpr float GRID_MAX = 100000.0f;

static int hex_nibble_(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

// Decode %XX into out. Returns false on a truncated or non-hex escape. '+' stays '+'.
static bool url_decode_(const char *in, size_t n, char *out, size_t cap) {
  size_t w = 0;
  for (size_t i = 0; i < n; i++) {
    char c = in[i];
    if (c == '%') {
      if (i + 2 >= n)
        return false;
      const int hi = hex_nibble_(in[i + 1]);
      const int lo = hex_nibble_(in[i + 2]);
      if (hi < 0 || lo < 0)
        return false;
      c = static_cast<char>((hi << 4) | lo);
      i += 2;
    }
    if (w + 1 >= cap)
      return false;
    out[w++] = c;
  }
  out[w] = 0;
  return true;
}

static bool parse_number_(const char *s, float *out) {
  if (s == nullptr)
    return false;
  while (*s == ' ' || *s == '\t')
    s++;
  if (*s == 0)
    return false;
  char *end = nullptr;
  const float v = std::strtof(s, &end);
  if (end == s)
    return false;
  while (*end == ' ' || *end == '\t')
    end++;
  if (*end != 0)
    return false;
  if (!std::isfinite(v))
    return false;
  *out = v;
  return true;
}

static bool in_range_(float v, float lo, float hi) { return v >= lo && v <= hi; }

SetQuery parse_set_fields(const char *room_value, const char *grid_value) {
  SetQuery q;
  if (room_value == nullptr && grid_value == nullptr) {
    q.error = "missing room_temp or grid_power";
    return q;
  }
  float room = 0;
  float grid = 0;
  if (room_value != nullptr) {
    if (!parse_number_(room_value, &room)) {
      q.error = "bad room_temp";
      return q;
    }
    if (!in_range_(room, ROOM_MIN, ROOM_MAX)) {
      q.error = "room_temp out of range";
      return q;
    }
  }
  if (grid_value != nullptr) {
    if (!parse_number_(grid_value, &grid)) {
      q.error = "bad grid_power";
      return q;
    }
    if (!in_range_(grid, GRID_MIN, GRID_MAX)) {
      q.error = "grid_power out of range";
      return q;
    }
  }
  if (room_value != nullptr) {
    q.has_room = true;
    q.room_c = room;
  }
  if (grid_value != nullptr) {
    q.has_grid = true;
    q.grid_w = grid;
  }
  q.ok = true;
  return q;
}

static SetQuery bad_query_(const char *error) {
  SetQuery bad;
  bad.error = error;
  return bad;
}

SetQuery parse_set_query(const char *query) {
  if (query == nullptr || query[0] == 0)
    return parse_set_fields(nullptr, nullptr);
  const char *room = nullptr;
  const char *grid = nullptr;
  char room_buf[48];
  char grid_buf[48];
  const char *p = query;
  while (*p) {
    const char *amp = std::strchr(p, '&');
    const size_t len = amp ? static_cast<size_t>(amp - p) : std::strlen(p);
    const char *eq = static_cast<const char *>(std::memchr(p, '=', len));
    if (eq == nullptr || eq == p) {
      if (len > 0)
        return bad_query_("bad parameter");
    } else {
      const size_t key_n = static_cast<size_t>(eq - p);
      const char *val = eq + 1;
      const size_t val_n = len - key_n - 1;
      char key[32];
      if (key_n >= sizeof(key) || !url_decode_(p, key_n, key, sizeof(key)))
        return bad_query_("bad parameter");
      if (std::strcmp(key, "room_temp") == 0) {
        if (!url_decode_(val, val_n, room_buf, sizeof(room_buf)))
          return bad_query_("bad room_temp");
        room = room_buf;
      } else if (std::strcmp(key, "grid_power") == 0) {
        if (!url_decode_(val, val_n, grid_buf, sizeof(grid_buf)))
          return bad_query_("bad grid_power");
        grid = grid_buf;
      }
    }
    if (!amp)
      break;
    p = amp + 1;
  }
  return parse_set_fields(room, grid);
}

}  // namespace web_set
}  // namespace esphome
