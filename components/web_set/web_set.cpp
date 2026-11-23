// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "web_set.h"

#include "query.h"

#include "esphome/core/log.h"

namespace esphome {
namespace web_set {

static const char *const TAG = "web_set";

void WebSet::setup() {
  this->base_->add_handler(this);
  ESP_LOGCONFIG(TAG, "GET /set?room_temp=…  /set?grid_power=…");
}

bool WebSet::canHandle(AsyncWebServerRequest *request) const {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  auto url = request->url_to(url_buf);
#else
  const auto &url = request->url();
#endif
  return request->method() == HTTP_GET && url == "/set";
}

void WebSet::handleRequest(AsyncWebServerRequest *request) {
  const bool has_room = request->hasParam("room_temp");
  const bool has_grid = request->hasParam("grid_power");
  const std::string room = has_room ? request->getParam("room_temp")->value() : std::string();
  const std::string grid = has_grid ? request->getParam("grid_power")->value() : std::string();
  const SetQuery q = parse_set_fields(has_room ? room.c_str() : nullptr, has_grid ? grid.c_str() : nullptr);
  if (!q.ok) {
    request->send(400, "text/plain", q.error != nullptr ? q.error : "bad request");
    return;
  }
  if ((q.has_room && this->room_ == nullptr) || (q.has_grid && this->grid_ == nullptr)) {
    request->send(400, "text/plain", "not available");
    return;
  }
  // Apply on the main loop, the same way the stock /number/.../set handler does.
  const float room_c = q.room_c;
  const float grid_w = q.grid_w;
  const bool set_room = q.has_room;
  const bool set_grid = q.has_grid;
  this->defer([this, room_c, grid_w, set_room, set_grid]() {
    if (set_room) {
      auto call = this->room_->make_call();
      call.set_value(room_c);
      call.perform();
      ESP_LOGI(TAG, "Room temperature %.1f", room_c);
    }
    if (set_grid) {
      auto call = this->grid_->make_call();
      call.set_value(grid_w);
      call.perform();
      ESP_LOGI(TAG, "Grid power %.0f W", grid_w);
    }
  });
  request->send(200, "text/plain", "ok");
}

}  // namespace web_set
}  // namespace esphome
