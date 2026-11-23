// Host tests for GET /set query parsing.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "../components/web_set/query.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using esphome::web_set::parse_set_fields;
using esphome::web_set::parse_set_query;

static int g_failed = 0;

static void expect(bool cond, const char *msg) {
  if (!cond) {
    std::printf("FAIL %s\n", msg);
    g_failed++;
  }
}

int main() {
  auto q = parse_set_query("room_temp=21.5");
  expect(q.ok && q.has_room && !q.has_grid && std::fabs(q.room_c - 21.5f) < 0.001f, "room 21.5");

  q = parse_set_query("grid_power=-1200");
  expect(q.ok && q.has_grid && !q.has_room && q.grid_w == -1200.0f, "grid -1200");

  q = parse_set_query("room_temp=18&grid_power=400");
  expect(q.ok && q.has_room && q.has_grid && q.room_c == 18.0f && q.grid_w == 400.0f, "both");

  q = parse_set_query("room_temp=%2D3.5");
  expect(q.ok && q.has_room && std::fabs(q.room_c - -3.5f) < 0.001f, "percent minus");

  q = parse_set_query("ignored=1&room_temp=20");
  expect(q.ok && q.has_room && q.room_c == 20.0f, "unknown key ignored");

  q = parse_set_query("");
  expect(!q.ok && q.error != nullptr && std::strstr(q.error, "missing") != nullptr, "empty");
  q = parse_set_query(nullptr);
  expect(!q.ok, "null");

  q = parse_set_query("room_temp=");
  expect(!q.ok && std::strcmp(q.error, "bad room_temp") == 0, "empty room");
  q = parse_set_query("room_temp=abc");
  expect(!q.ok && std::strcmp(q.error, "bad room_temp") == 0, "text room");
  q = parse_set_query("room_temp=21.5C");
  expect(!q.ok, "trailing junk");
  q = parse_set_query("room_temp=nan");
  expect(!q.ok, "nan");
  q = parse_set_query("room_temp=inf");
  expect(!q.ok, "inf");
  q = parse_set_query("room_temp=-30");
  expect(q.ok && q.room_c == -30.0f, "room min");
  q = parse_set_query("room_temp=60");
  expect(q.ok && q.room_c == 60.0f, "room max");
  q = parse_set_query("room_temp=-30.1");
  expect(!q.ok && std::strcmp(q.error, "room_temp out of range") == 0, "room low");
  q = parse_set_query("room_temp=60.1");
  expect(!q.ok && std::strcmp(q.error, "room_temp out of range") == 0, "room high");

  q = parse_set_query("grid_power=-100000");
  expect(q.ok && q.grid_w == -100000.0f, "grid min");
  q = parse_set_query("grid_power=100000");
  expect(q.ok && q.grid_w == 100000.0f, "grid max");
  q = parse_set_query("grid_power=-100001");
  expect(!q.ok && std::strcmp(q.error, "grid_power out of range") == 0, "grid low");
  q = parse_set_query("grid_power=nope");
  expect(!q.ok && std::strcmp(q.error, "bad grid_power") == 0, "bad grid");

  q = parse_set_query("room_temp=21&grid_power=bad");
  expect(!q.ok && !q.has_room, "invalid grid rejects the whole query");

  q = parse_set_query("room_temp=%");
  expect(!q.ok && std::strcmp(q.error, "bad room_temp") == 0, "bad escape");
  q = parse_set_query("noroom");
  expect(!q.ok && std::strcmp(q.error, "bad parameter") == 0, "no equals");

  q = parse_set_fields("21.5", nullptr);
  expect(q.ok && q.has_room && std::fabs(q.room_c - 21.5f) < 0.001f, "fields room");
  q = parse_set_fields(nullptr, nullptr);
  expect(!q.ok, "fields none");
  q = parse_set_fields("", "-10");
  expect(!q.ok && std::strcmp(q.error, "bad room_temp") == 0, "fields empty room");

  if (g_failed) {
    std::printf("%d failed\n", g_failed);
    return 1;
  }
  std::printf("web_set query tests passed\n");
  return 0;
}
