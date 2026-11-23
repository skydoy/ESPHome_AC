// Parse GET /set?room_temp=…&grid_power=… (no ESPHome headers: host-tested).
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

namespace esphome {
namespace web_set {

struct SetQuery {
  bool ok{false};
  bool has_room{false};
  float room_c{0};
  bool has_grid{false};
  float grid_w{0};
  const char *error{nullptr};  // static string when ok is false
};

/// `query` is the part after '?', already split from the path. nullptr or "" is an error.
/// Accepts room_temp (-30..60 °C) and grid_power (-100000..100000 W). Unknown keys are ignored.
/// A known key with a bad or out-of-range value fails the whole query (nothing is applied).
SetQuery parse_set_query(const char *query);

/// Same checks, with values the HTTP server has already split out.
/// Pass nullptr for a key that was not present. An empty string is a bad value.
SetQuery parse_set_fields(const char *room_value, const char *grid_value);

}  // namespace web_set
}  // namespace esphome
