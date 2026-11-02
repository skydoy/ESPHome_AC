// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "ac_setup.h"

#include <cstdlib>
#include <cstring>

#include <nvs.h>

#include "esphome/components/network/util.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ac_setup {

static const char *const TAG = "ac_setup";
static constexpr uint32_t CHECK_TIMEOUT_MS = 45000;  // manifest check before "no internet"
static constexpr size_t MAX_BODY = 256;

// ---- UART adapter --------------------------------------------------------------------------
void UartBus::configure(const UartProfile &p) {
  this->uart_->flush();
  this->uart_->set_baud_rate(p.baud);
  this->uart_->set_data_bits(8);
  this->uart_->set_stop_bits(1);
  this->uart_->set_parity(p.parity == 'E' ? uart::UART_CONFIG_PARITY_EVEN : uart::UART_CONFIG_PARITY_NONE);
  this->uart_->load_settings(false);
}

size_t UartBus::read(uint8_t *buf, size_t cap) {
  const size_t avail = this->uart_->available();
  if (avail == 0)
    return 0;
  const size_t n = avail < cap ? avail : cap;
  return this->uart_->read_array(buf, n) ? n : 0;
}

void UartBus::write(const uint8_t *data, size_t len) { this->uart_->write_array(data, len); }

// ---- Component -----------------------------------------------------------------------------
void AcSetup::setup() {
  this->base_->init();
  this->base_->add_handler(this);
  ota::get_global_ota_callback()->add_global_state_listener(this);
  ESP_LOGI(TAG, "Setup firmware ready: open http://<device-ip>/setup");
}

void AcSetup::dump_config() {
  ESP_LOGCONFIG(TAG,
                "AC setup:\n"
                "  Board: %s\n"
                "  Firmware: %s\n"
                "  Bus: %s %s\n"
                "  Install origin: %s\n"
                "  Probe allow-list: 0x0A, 0x66/00, 0x07 (read-only)",
                this->board_, this->fw_, this->phys_, this->pins_, this->allowed_origin_);
}

void AcSetup::loop() {
  LockGuard guard(this->lock_);
  const uint32_t now = millis();
  if (!this->auto_started_ && now >= this->auto_start_ms_) {
    this->auto_started_ = true;
    if (this->engine_.state() == State::IDLE && this->engine_.probe_id() == 0) {
      ESP_LOGI(TAG, "Starting the AC probe");
      this->engine_.start(now, false);
    }
  }
  this->engine_.tick(now);
  if (this->engine_.busy())
    this->high_freq_.start();
  else
    this->high_freq_.stop();

  if (this->engine_.state() == State::VALIDATE_OK && this->phase_ == Phase::NONE)
    this->begin_install_(now);
  if (this->phase_ == Phase::CHECKING)
    this->poll_install_(now);

  if (this->engine_.seq() != this->last_logged_seq_) {
    this->last_logged_seq_ = this->engine_.seq();
    // Machine-readable progress for the installer's serial console.
    ESP_LOGI(TAG, "@@%s", this->engine_.state_json().c_str());
  }
}

void AcSetup::begin_install_(uint32_t now) {
  ESP_LOGI(TAG, "Validation OK, looking for the %s firmware", HISENSE_XM.image_key);
  this->phase_ = Phase::CHECKING;
  this->check_started_ = now;
  this->engine_.set_install_state(State::INSTALL, 0, "Checking for the firmware");
  this->update_->check();
}

void AcSetup::poll_install_(uint32_t now) {
  const auto &info = this->update_->update_info;
  const bool ready = this->update_->state == update::UPDATE_STATE_AVAILABLE && !info.firmware_url.empty();
  if (!ready) {
    if (now - this->check_started_ > CHECK_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Could not read the firmware manifest (no internet?)");
      this->phase_ = Phase::NONE;
      this->engine_.set_install_state(State::NO_INTERNET, 0, "Could not reach the firmware download (internet?)");
    }
    return;
  }
  if (info.firmware_url.compare(0, std::strlen(this->allowed_origin_), this->allowed_origin_) != 0) {
    ESP_LOGE(TAG, "Refusing firmware from an unexpected origin: %s", info.firmware_url.c_str());
    this->phase_ = Phase::NONE;
    this->engine_.set_install_state(State::INSTALL_FAILED, 0, "Firmware URL outside the project site");
    return;
  }
  if (!this->write_detect_record_())
    ESP_LOGW(TAG, "Could not store the detection record (continuing)");
  ESP_LOGI(TAG, "Installing %s firmware %s", HISENSE_XM.image_key, info.latest_version.c_str());
  this->phase_ = Phase::FLASHING;
  this->last_ota_pct_ = -1;
  std::string msg = "Downloading firmware " + info.latest_version;
  this->engine_.set_install_state(State::INSTALL, 0, msg.c_str());
  this->update_->perform(true);
}

bool AcSetup::write_detect_record_() {
  // Read by future brand firmware to skip re-detection. Namespace "esphome_ac", key "detect".
  char rec[192];
  std::snprintf(rec, sizeof(rec),
                "{\"schema\":1,\"driver\":\"%s\",\"type\":\"%02X %02X\",\"uart\":\"%s\",\"setup_fw\":\"%s\","
                "\"board\":\"%s\"}",
                HISENSE_XM.id, this->engine_.type_hi(), this->engine_.type_lo(), PROFILES[0].name, this->fw_,
                this->board_);
  nvs_handle_t h;
  if (nvs_open("esphome_ac", NVS_READWRITE, &h) != ESP_OK)
    return false;
  bool ok = nvs_set_str(h, "detect", rec) == ESP_OK && nvs_commit(h) == ESP_OK;
  nvs_close(h);
  return ok;
}

void AcSetup::on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) {
  (void) comp;
  LockGuard guard(this->lock_);
  if (this->phase_ != Phase::FLASHING)
    return;
  switch (state) {
    case ota::OTA_STARTED:
      this->engine_.set_install_state(State::INSTALL, 0, "Downloading firmware");
      break;
    case ota::OTA_IN_PROGRESS: {
      const int pct = progress < 0 ? 0 : progress > 100 ? 100 : static_cast<int>(progress);
      if (pct != this->last_ota_pct_) {
        this->last_ota_pct_ = pct;
        this->engine_.set_install_state(State::INSTALL, static_cast<uint8_t>(pct), "Downloading firmware");
      }
      break;
    }
    case ota::OTA_COMPLETED:
      this->engine_.set_install_state(State::REBOOT, 100, "Installed, rebooting into the AC firmware");
      break;
    case ota::OTA_ERROR:
    case ota::OTA_ABORT: {
      char msg[96];
      std::snprintf(msg, sizeof(msg), "Download or flash failed (error %u); still on the setup firmware",
                    static_cast<unsigned>(error));
      this->phase_ = Phase::NONE;
      this->engine_.set_install_state(State::INSTALL_FAILED, 0, msg);
      break;
    }
    default:
      return;
  }
  ESP_LOGI(TAG, "@@%s", this->engine_.state_json().c_str());
}

// ---- Screen / LED --------------------------------------------------------------------------
uint32_t AcSetup::screen_seq() {
  LockGuard guard(this->lock_);
  return this->engine_.seq();
}

int AcSetup::screen_pct() {
  LockGuard guard(this->lock_);
  const State st = this->engine_.state();
  return (st == State::INSTALL || st == State::REBOOT) ? this->engine_.install_pct() : -1;
}

bool AcSetup::ac_link() {
  LockGuard guard(this->lock_);
  return this->engine_.detected();
}

bool AcSetup::ac_fault() {
  LockGuard guard(this->lock_);
  switch (this->engine_.state()) {
    case State::NO_REPLY:
    case State::UNKNOWN:
    case State::BUS_BUSY:
    case State::VALIDATE_FAILED:
    case State::INSTALL_FAILED:
    case State::NO_INTERNET:
      return true;
    default:
      return false;
  }
}

void AcSetup::screen_text(std::string *title, std::string *detail, uint8_t *level) {
  LockGuard guard(this->lock_);
  *level = 0;
  *detail = "";
  switch (this->engine_.state()) {
    case State::IDLE:
      *title = this->engine_.detected() ? "Waiting" : "Ready";
      *detail = "Open /setup in a browser";
      break;
    case State::LISTEN:
      *title = "Listening";
      *detail = "Passive, nothing sent";
      break;
    case State::IDENTIFY:
      *title = "Asking the AC";
      *detail = step_str(this->engine_.step());
      break;
    case State::SCAN:
      *title = "Scanning";
      *detail = "Passive capture";
      break;
    case State::CANDIDATE:
      *title = "Found: Hisense";
      *detail = "Confirm on /setup";
      *level = 1;
      break;
    case State::VALIDATE:
      *title = "Validating";
      break;
    case State::VALIDATE_OK:
      *title = "Validated";
      *level = 1;
      break;
    case State::VALIDATE_FAILED:
      *title = "Validation failed";
      *detail = "Retry on /setup";
      *level = 3;
      break;
    case State::INSTALL: {
      char b[24];
      std::snprintf(b, sizeof(b), "Installing %u%%", this->engine_.install_pct());
      *title = b;
      *detail = "Do not unplug";
      *level = 1;
      break;
    }
    case State::INSTALL_FAILED:
      *title = "Install failed";
      *detail = "Retry on /setup";
      *level = 3;
      break;
    case State::NO_INTERNET:
      *title = "No internet";
      *detail = "Check Wi-Fi, retry";
      *level = 3;
      break;
    case State::REBOOT:
      *title = "Rebooting";
      *level = 1;
      break;
    case State::UNKNOWN:
      *title = "Unknown AC";
      *detail = "Report it on /setup";
      *level = 2;
      break;
    case State::NO_REPLY:
      *title = "No reply";
      *detail = "Check wiring, A/B";
      *level = 3;
      break;
    case State::BUS_BUSY:
      *title = "Bus busy";
      *detail = "Unplug the Wi-Fi module";
      *level = 2;
      break;
  }
}

// ---- Web API -------------------------------------------------------------------------------
bool AcSetup::canHandle(AsyncWebServerRequest *request) const {
  const auto m = request->method();
  if (m != HTTP_GET && m != HTTP_POST)
    return false;
  char buf[AsyncWebServerRequest::URL_BUF_SIZE];
  const StringRef url = request->url_to(buf);
  return url == StringRef("/setup") || url == StringRef("/setup/") || url.starts_with("/api/setup/");
}

bool AcSetup::origin_ok_(AsyncWebServerRequest *request) const {
  // Browsers send Origin on cross-site POSTs; it must be this device. Tools (curl) send none.
  auto origin = request->get_header("Origin");
  if (!origin.has_value())
    return true;
  auto host = request->get_header("Host");
  if (!host.has_value())
    return false;
  return *origin == "http://" + *host;
}

void AcSetup::send_json_(AsyncWebServerRequest *request, int code, const std::string &body) {
  auto *rsp = request->beginResponse(code, "application/json", body);
  rsp->addHeader("Cache-Control", "no-store");
  request->send(rsp);
}

void AcSetup::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  (void) request;
  (void) total;
  if (index == 0)
    this->body_.clear();
  if (this->body_.size() + len <= MAX_BODY)
    this->body_.append(reinterpret_cast<const char *>(data), len);
}

static bool json_find_uint(const std::string &s, const char *key, uint32_t *out) {
  const std::string k = std::string("\"") + key + "\"";
  size_t p = s.find(k);
  if (p == std::string::npos)
    return false;
  p = s.find(':', p + k.size());
  if (p == std::string::npos)
    return false;
  const char *c = s.c_str() + p + 1;
  while (*c == ' ')
    c++;
  char *end = nullptr;
  const unsigned long v = std::strtoul(c, &end, 10);
  if (end == c)
    return false;
  *out = static_cast<uint32_t>(v);
  return true;
}

static bool json_find_str(const std::string &s, const char *key, std::string *out) {
  const std::string k = std::string("\"") + key + "\"";
  size_t p = s.find(k);
  if (p == std::string::npos)
    return false;
  p = s.find('"', s.find(':', p + k.size()));
  if (p == std::string::npos)
    return false;
  const size_t e = s.find('"', p + 1);
  if (e == std::string::npos)
    return false;
  *out = s.substr(p + 1, e - p - 1);
  return true;
}

std::string AcSetup::info_json_() const {
  return std::string("{\"fw\":\"") + json_escape(this->fw_) + "\",\"board\":\"" + json_escape(this->board_) +
         "\",\"phys\":\"" + this->phys_ + "\",\"pins\":\"" + json_escape(this->pins_) +
         "\",\"network\":" + (network::is_connected() ? "true" : "false") + ",\"drivers\":[{\"id\":\"" +
         HISENSE_XM.id + "\",\"label\":\"" + json_escape(HISENSE_XM.label) + "\",\"phys\":\"" + HISENSE_XM.phys +
         "\"}],\"allow_list\":[\"0x0A\",\"0x66/00\",\"0x07\"],\"catalog\":\"fixed-manifest\"}";
}

void AcSetup::handleRequest(AsyncWebServerRequest *request) {
  char buf[AsyncWebServerRequest::URL_BUF_SIZE];
  const StringRef ref = request->url_to(buf);
  const std::string url(ref.c_str(), ref.size());
  const bool post = request->method() == HTTP_POST;

  if (url == "/setup" || url == "/setup/") {
    if (post) {
      request->send(405);
      return;
    }
    auto *rsp = request->beginResponse(200, "text/html", this->page_, this->page_len_);
    rsp->addHeader("Content-Encoding", "gzip");
    rsp->addHeader("Cache-Control", "no-store");
    request->send(rsp);
    return;
  }

  const std::string ep = url.substr(std::strlen("/api/setup/"));
  if (post) {
    std::string body;
    body.swap(this->body_);
    if (!this->origin_ok_(request)) {
      this->send_json_(request, 403, "{\"error\":\"origin\"}");
      return;
    }
    auto ctype = request->get_header("Content-Type");
    if (!ctype.has_value() || ctype->rfind("application/json", 0) != 0) {
      this->send_json_(request, 415, "{\"error\":\"content-type must be application/json\"}");
      return;
    }
    LockGuard guard(this->lock_);
    const uint32_t now = millis();
    if (ep == "probe") {
      const bool passive = body.find("\"passive_only\":true") != std::string::npos;
      const uint32_t id = this->engine_.start(now, passive);
      this->auto_started_ = true;
      if (id == 0) {
        this->send_json_(request, 409, "{\"error\":\"busy\"}");
        return;
      }
      this->send_json_(request, 202, "{\"probe_id\":" + std::to_string(id) + "}");
      return;
    }
    if (ep == "confirm") {
      uint32_t id = 0;
      std::string driver;
      if (!json_find_uint(body, "probe_id", &id) || !json_find_str(body, "driver", &driver)) {
        this->send_json_(request, 400, "{\"error\":\"probe_id and driver required\"}");
        return;
      }
      const State st = this->engine_.state();
      if ((st == State::INSTALL_FAILED || st == State::NO_INTERNET) && id == this->engine_.probe_id() &&
          driver == HISENSE_XM.id) {
        this->engine_.set_install_state(State::VALIDATE_OK, 0, "");  // retry the install
        this->send_json_(request, 202, "{\"ok\":true}");
        return;
      }
      if (!this->engine_.confirm(now, id, driver.c_str())) {
        this->send_json_(request, 409, "{\"error\":\"no matching candidate\"}");
        return;
      }
      this->send_json_(request, 202, "{\"ok\":true}");
      return;
    }
    if (ep == "cancel") {
      this->engine_.cancel(now);
      this->send_json_(request, 200, "{\"ok\":true}");
      return;
    }
    this->send_json_(request, 404, "{\"error\":\"not found\"}");
    return;
  }

  if (ep == "info") {
    this->send_json_(request, 200, this->info_json_());
    return;
  }
  // Build the reply under the lock, send it after releasing it (slow clients must not stall the probe).
  std::string out;
  const char *ctype = "application/json";
  const char *disposition = nullptr;
  {
    LockGuard guard(this->lock_);
    if (ep == "state") {
      out = this->engine_.state_json();
    } else if (ep == "result") {
      out = this->engine_.result_json(this->fw_, this->board_);
    } else if (ep == "report") {
      out = "{\"url\":\"" + json_escape(this->engine_.report_url(this->fw_, this->board_)) + "\"}";
    } else if (ep == "capture.json") {
      out = this->engine_.capture_json();
      disposition = "attachment; filename=\"esphome-ac-capture.json\"";
    } else if (ep == "capture.txt") {
      out = this->engine_.capture_text(this->fw_, this->board_);
      ctype = "text/plain";
      disposition = "attachment; filename=\"esphome-ac-capture.txt\"";
    } else if (ep == "capture.bin") {
      // Chunk records as stored: [t u32 LE][dir][profile][len u16 LE][data].
      ChunkView v{};
      for (size_t i = 0; this->engine_.capture().get(i, &v); i++) {
        const uint8_t h[8] = {static_cast<uint8_t>(v.t_ms),       static_cast<uint8_t>(v.t_ms >> 8),
                              static_cast<uint8_t>(v.t_ms >> 16), static_cast<uint8_t>(v.t_ms >> 24),
                              static_cast<uint8_t>(v.dir),        v.profile,
                              static_cast<uint8_t>(v.len & 0xFF), static_cast<uint8_t>(v.len >> 8)};
        out.append(reinterpret_cast<const char *>(h), sizeof(h));
        out.append(reinterpret_cast<const char *>(v.data), v.len);
      }
      ctype = "application/octet-stream";
      disposition = "attachment; filename=\"esphome-ac-capture.bin\"";
    } else {
      out.clear();
      ctype = nullptr;
    }
  }
  if (ctype == nullptr) {
    this->send_json_(request, 404, "{\"error\":\"not found\"}");
    return;
  }
  auto *rsp = request->beginResponse(200, ctype, out);
  rsp->addHeader("Cache-Control", "no-store");
  if (disposition != nullptr)
    rsp->addHeader("Content-Disposition", disposition);
  request->send(rsp);
}

}  // namespace ac_setup
}  // namespace esphome
