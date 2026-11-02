// Setup firmware glue: runs the probe engine on the AC UART, serves /setup and /api/setup/*,
// and installs the detected brand firmware through an http_request update entity.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <string>

#include "esphome/components/ota/ota_backend.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/update/update_entity.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "probe_engine.h"

namespace esphome {
namespace ac_setup {

// Bus adapter: the probe engine only ever sees this interface.
class UartBus : public Bus {
 public:
  explicit UartBus(uart::UARTComponent *uart) : uart_(uart) {}
  void configure(const UartProfile &p) override;
  size_t read(uint8_t *buf, size_t cap) override;
  void write(const uint8_t *data, size_t len) override;

 protected:
  uart::UARTComponent *uart_;
};

class AcSetup : public Component, public AsyncWebHandler, public ota::OTAGlobalStateListener {
 public:
  AcSetup(web_server_base::WebServerBase *base, uart::UARTComponent *uart, update::UpdateEntity *update)
      : base_(base), bus_(uart), update_(update), engine_(&bus_) {}

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }

  void set_board(const char *b) { this->board_ = b; }
  void set_firmware_version(const char *v) { this->fw_ = v; }
  void set_allowed_origin(const char *o) { this->allowed_origin_ = o; }
  void set_phys(const char *p) { this->phys_ = p; }
  void set_pins(const char *p) { this->pins_ = p; }
  void set_auto_start_delay(uint32_t ms) { this->auto_start_ms_ = ms; }
  void set_page(const uint8_t *data, size_t len) {
    this->page_ = data;
    this->page_len_ = len;
  }

  // AsyncWebHandler
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
  bool isRequestHandlerTrivial() const override { return false; }

  // ota::OTAGlobalStateListener: install progress (runs while the OTA blocks the main loop).
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) override;

  // For the status screen / LED (main loop only).
  uint32_t screen_seq();
  void screen_text(std::string *title, std::string *detail, uint8_t *level);
  // Install progress 0..100 while installing or rebooting, -1 otherwise.
  int screen_pct();
  bool ac_link();
  bool ac_fault();

 protected:
  enum class Phase : uint8_t { NONE, CHECKING, FLASHING };

  void begin_install_(uint32_t now);
  void poll_install_(uint32_t now);
  bool write_detect_record_();
  bool origin_ok_(AsyncWebServerRequest *request) const;
  void send_json_(AsyncWebServerRequest *request, int code, const std::string &body);
  std::string info_json_() const;

  web_server_base::WebServerBase *base_;
  UartBus bus_;
  update::UpdateEntity *update_;
  Engine engine_;
  Mutex lock_;
  HighFrequencyLoopRequester high_freq_;
  const char *board_{""};
  const char *fw_{""};
  const char *allowed_origin_{""};
  const char *phys_{"rs485"};
  const char *pins_{""};
  uint32_t auto_start_ms_{10000};
  bool auto_started_{false};
  const uint8_t *page_{nullptr};
  size_t page_len_{0};
  Phase phase_{Phase::NONE};
  uint32_t check_started_{0};
  uint32_t last_logged_seq_{0};
  int last_ota_pct_{-1};
  std::string body_;
};

}  // namespace ac_setup
}  // namespace esphome
