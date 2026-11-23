// GET /set?room_temp=21.5&grid_power=-1200 on the existing web server.
// Shelly H&T webhooks are GET; ESPHome's /number/<name>/set only applies a value on POST.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "esphome/components/web_server_base/web_server_base.h"

namespace esphome {
namespace web_set {

class WebSet : public Component, public AsyncWebHandler {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }

  void set_web_server_base(web_server_base::WebServerBase *base) { this->base_ = base; }
  void set_room_number(number::Number *n) { this->room_ = n; }
  void set_grid_number(number::Number *n) { this->grid_ = n; }

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  web_server_base::WebServerBase *base_{nullptr};
  number::Number *room_{nullptr};
  number::Number *grid_{nullptr};
};

}  // namespace web_set
}  // namespace esphome
