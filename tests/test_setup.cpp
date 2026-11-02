// Host unit tests for components/ac_setup/probe_engine.{h,cpp} (the setup firmware's probe).
// Build and run:  make -C tests
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "../components/ac_setup/probe_engine.h"
#include "../components/hisense_ac/hisense_command.h"

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

using namespace esphome::ac_setup;
namespace proto = esphome::hisense_ac::proto;

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (cond) {                                                         \
      g_passed++;                                                       \
    } else {                                                            \
      g_failed++;                                                       \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                   \
  } while (0)

using Bytes = std::vector<uint8_t>;

static bool contains(const std::string &s, const char *needle) { return s.find(needle) != std::string::npos; }

// A reply frame from the AC (dir 01), sealed and byte-stuffed for the wire.
static Bytes reply(uint8_t cls, uint8_t sub, size_t len) {
  Bytes f(len, 0);
  f[2] = proto::DIR_FROM_AC;
  f[3] = 0x40;
  f[13] = cls;
  f[14] = sub;
  if (cls == proto::CLASS_DEVTYPE) {
    f[16] = 0x01;
    f[17] = 0x01;
  }
  proto::seal(f.data(), len);
  Bytes w(2 * len);
  w.resize(proto::stuff(f.data(), len, w.data(), w.size()));
  return w;
}

static Bytes wire_of(const uint8_t *f, size_t n) {
  Bytes w(2 * n + 4);
  w.resize(proto::stuff(f, n, w.data(), w.size()));
  return w;
}

// Fake RS-485 bus: records writes; a responder schedules RX bytes at a later time.
struct FakeBus : public Bus {
  enum class Ac { SILENT, DEVTYPE, STATUS_ONLY, ECHO_ONLY, GARBAGE, FOREIGN, VALIDATE_SILENT };
  Ac ac{Ac::SILENT};
  bool echo{false};
  uint32_t now{0};
  std::deque<std::pair<uint32_t, Bytes>> rx;
  std::vector<Bytes> writes;
  std::vector<uint8_t> classes;
  int configs{0};
  const UartProfile *profile{nullptr};
  uint32_t validate_frames{0};

  void configure(const UartProfile &p) override {
    this->configs++;
    this->profile = &p;
  }
  size_t read(uint8_t *buf, size_t cap) override {
    if (this->rx.empty() || static_cast<int32_t>(this->rx.front().first - this->now) > 0)
      return 0;
    Bytes &b = this->rx.front().second;
    const size_t n = b.size() < cap ? b.size() : cap;
    std::memcpy(buf, b.data(), n);
    b.erase(b.begin(), b.begin() + static_cast<long>(n));
    if (b.empty())
      this->rx.pop_front();
    return n;
  }
  void push(uint32_t at, const Bytes &b) {
    auto it = this->rx.begin();
    while (it != this->rx.end() && static_cast<int32_t>(it->first - at) <= 0)
      ++it;
    this->rx.insert(it, {at, b});
  }
  void write(const uint8_t *data, size_t len) override {
    Bytes w(data, data + len);
    this->writes.push_back(w);
    uint8_t dec[256];
    const size_t dn = unstuff(data, len, dec, sizeof(dec));
    const uint8_t cls = dn > 13 ? dec[13] : 0;
    this->classes.push_back(cls);
    if (this->echo || this->ac == Ac::ECHO_ONLY)
      this->push(this->now, w);
    const uint32_t at = this->now + 120;
    switch (this->ac) {
      case Ac::DEVTYPE:
        if (cls == proto::CLASS_DEVTYPE)
          this->push(at, reply(proto::CLASS_DEVTYPE, 0x04, 22));
        else if (cls == proto::CLASS_STATUS)
          this->push(at, reply(proto::CLASS_STATUS, 0x01, 82));
        else if (cls == proto::CLASS_VERSION)
          this->push(at, reply(proto::CLASS_VERSION, 0x01, 30));
        break;
      case Ac::VALIDATE_SILENT:
        if (cls == proto::CLASS_DEVTYPE)
          this->push(at, reply(proto::CLASS_DEVTYPE, 0x04, 22));
        else if (cls == proto::CLASS_VERSION)
          this->push(at, reply(proto::CLASS_VERSION, 0x01, 30));
        break;
      case Ac::STATUS_ONLY:
        if (cls == proto::CLASS_STATUS)
          this->push(at, reply(proto::CLASS_STATUS, 0x01, 82));
        break;
      case Ac::GARBAGE:
        this->push(at, Bytes{0x13, 0x77, 0xA5, 0x0F, 0xFF, 0x3C, 0x00, 0x81});
        break;
      default:
        break;
    }
  }
};

static void run(Engine &e, FakeBus &bus, uint32_t &now, uint32_t ms, uint32_t step = 5) {
  for (uint32_t t = 0; t < ms; t += step) {
    now += step;
    bus.now = now;
    e.tick(now);
  }
}

static void run_until_done(Engine &e, FakeBus &bus, uint32_t &now, uint32_t max_ms = 60000) {
  for (uint32_t t = 0; t < max_ms && e.busy(); t += 5) {
    now += 5;
    bus.now = now;
    e.tick(now);
  }
}

// ---- T-U-1: probe scenarios -------------------------------------------------------------------
static void test_detect_devtype() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::DEVTYPE;
  bus.echo = true;  // auto-direction transceiver
  Engine e(&bus);
  uint32_t now = 1000;
  const uint32_t id = e.start(now, false);
  CHECK(id == 1);
  CHECK(e.state() == State::LISTEN);
  CHECK(e.start(now, false) == 0);  // busy: second start refused (HTTP 409)
  run(e, bus, now, 2900);
  CHECK(bus.writes.empty());  // P1 is passive
  run_until_done(e, bus, now);
  CHECK(e.state() == State::CANDIDATE);
  CHECK(e.detected());
  CHECK(std::string(e.outcome()) == "detected");
  CHECK(e.type_hi() == 0x01 && e.type_lo() == 0x01);
  CHECK(bus.classes.size() == 2);
  CHECK(bus.classes.size() >= 1 && bus.classes[0] == proto::CLASS_DEVTYPE);
  CHECK(bus.classes.size() >= 2 && bus.classes[1] == proto::CLASS_VERSION);
  CHECK(e.echo_seen());
  const std::string r = e.result_json("0.3.0", "m5stickc-plus2");
  CHECK(contains(r, "\"driver\":\"hisense_xm\""));
  CHECK(contains(r, "\"type\":\"01 01\""));
  CHECK(contains(r, "\"uart_errors\":null"));
  CHECK(!contains(r, "ab_swap_likely"));

  // Confirm with a wrong probe id or driver is refused.
  CHECK(!e.confirm(now, id + 1, "hisense_xm"));
  CHECK(!e.confirm(now, id, "midea"));
  CHECK(e.confirm(now, id, "hisense_xm"));
  CHECK(e.state() == State::VALIDATE);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::VALIDATE_OK);
  CHECK(e.has_status());
  CHECK(bus.classes.back() == proto::CLASS_STATUS);
  // Install progress is reported through the engine.
  e.set_install_state(State::INSTALL, 42, "Downloading");
  const std::string s = e.state_json();
  CHECK(contains(s, "\"state\":\"install\""));
  CHECK(contains(s, "\"pct\":42"));
  e.cancel(now);
  CHECK(e.state() == State::INSTALL);  // an OTA cannot be cancelled
  for (uint8_t c : bus.classes)
    CHECK(c == proto::CLASS_DEVTYPE || c == proto::CLASS_VERSION || c == proto::CLASS_STATUS);
}

static void test_detect_status_only() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::STATUS_ONLY;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::CANDIDATE);
  CHECK(e.detected());
  // 3 x 0x0A, then 0x66/00 answered, then one 0x07.
  CHECK(bus.classes.size() == 5);
  CHECK(bus.classes.size() == 5 && bus.classes[0] == 0x0A && bus.classes[2] == 0x0A && bus.classes[3] == 0x66 &&
        bus.classes[4] == 0x07);
  const std::string r = e.result_json("fw", "b");
  CHECK(contains(r, "reply to status request"));
  CHECK(contains(r, "no 0x0A reply"));
}

static void test_silence() {
  FakeBus bus;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::NO_REPLY);
  CHECK(std::string(e.outcome()) == "no_reply");
  CHECK(bus.writes.size() == 5);  // 3 x 0x0A + 2 x 0x66/00, no version request
  CHECK(bus.profile == &PROFILES[0]);  // back on the documented profile after the scan
  CHECK(bus.configs >= 1 + 4 + 1);
  CHECK(contains(e.result_json("fw", "b"), "silent_bus"));
}

static void test_echo_only() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::ECHO_ONLY;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::NO_REPLY || e.state() == State::UNKNOWN);
  CHECK(!e.detected());
  CHECK(e.echo_seen());
  const std::string r = e.result_json("fw", "b");
  CHECK(contains(r, "echo_only"));
  CHECK(!contains(r, "ab_swap_likely"));
  CHECK(!contains(r, "silent_bus"));
}

static void test_garbage_ab_swap() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::GARBAGE;
  bus.echo = true;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::UNKNOWN);
  CHECK(e.hint_ab_swap());
  CHECK(contains(e.result_json("fw", "b"), "ab_swap_likely"));
  CHECK(contains(e.capture_text("fw", "b"), "hints=ab_swap_likely"));
}

static void test_foreign_traffic() {
  FakeBus bus;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  // Another master (the stock module) polls during P1.
  uint8_t f[32];
  const size_t n = proto::build_status_request(0x01, 0x01, 0x00, f, sizeof(f));
  bus.push(500, wire_of(f, n));
  bus.push(1500, reply(proto::CLASS_STATUS, 0x01, 82));
  run_until_done(e, bus, now);
  CHECK(e.state() == State::BUS_BUSY);
  CHECK(std::string(e.outcome()) == "bus_busy");
  CHECK(bus.writes.empty());  // never transmits on a busy bus
}

static void test_passive_only() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::DEVTYPE;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, true);
  run_until_done(e, bus, now);
  CHECK(bus.writes.empty());
  CHECK(e.state() == State::NO_REPLY);
}

static void test_validate_failed_and_timeout() {
  FakeBus bus;
  bus.ac = FakeBus::Ac::VALIDATE_SILENT;
  Engine e(&bus);
  uint32_t now = 0;
  const uint32_t id = e.start(now, false);
  run_until_done(e, bus, now);
  CHECK(e.state() == State::CANDIDATE);
  CHECK(e.confirm(now, id, "hisense_xm"));
  run_until_done(e, bus, now);
  CHECK(e.state() == State::VALIDATE_FAILED);
  // The candidate (and a failed validation) expires after 5 minutes without a decision.
  run(e, bus, now, Engine::CANDIDATE_TIMEOUT_MS + 100, 100);
  CHECK(e.state() == State::IDLE);
  CHECK(!e.confirm(now, id, "hisense_xm"));
  CHECK(std::string(e.outcome()) == "detected");  // the result stays readable

  // Cancel during a probe returns to IDLE and the next start gets a new id.
  CHECK(e.start(now, false) == id + 1);
  run(e, bus, now, 100);
  e.cancel(now);
  CHECK(e.state() == State::IDLE);
  CHECK(!e.busy());
  CHECK(std::string(e.outcome()) == "cancelled");
}

// ---- T-U-2: transmit allow-list and rate limit ------------------------------------------------
static void test_allow_list() {
  uint8_t f[64];
  size_t n = proto::build_devtype_request(f, sizeof(f));
  Bytes w = wire_of(f, n);
  CHECK(tx_allowed(w.data(), w.size()));
  n = proto::build_version_request(0x01, 0x01, f, sizeof(f));
  w = wire_of(f, n);
  CHECK(tx_allowed(w.data(), w.size()));
  n = proto::build_status_request(0x01, 0x01, 0x00, f, sizeof(f));
  w = wire_of(f, n);
  CHECK(tx_allowed(w.data(), w.size()));

  // 0x66/0x40 (product type) is not on the list.
  n = proto::build_status_request(0x01, 0x01, 0x40, f, sizeof(f));
  w = wire_of(f, n);
  CHECK(!tx_allowed(w.data(), w.size()));
  // 0x1E heartbeat and 0x65 command are refused.
  uint8_t hb[64];
  n = proto::build_heartbeat(0x01, 0x01, true, hb, sizeof(hb));
  w = wire_of(hb, n);
  CHECK(n > 0 && !tx_allowed(w.data(), w.size()));
  proto::Command cmd{};
  uint8_t cf[64];
  proto::build_command(cmd, 0x01, 0x01, cf);
  w = wire_of(cf, 50);
  CHECK(cf[13] == 0x65);
  CHECK(!tx_allowed(w.data(), w.size()));
  // dir 01 (a reply) is refused.
  w = reply(proto::CLASS_STATUS, 0x00, 82);
  CHECK(!tx_allowed(w.data(), w.size()));
  // Bad checksum.
  n = proto::build_devtype_request(f, sizeof(f));
  f[n - 3] ^= 0x01;
  w = wire_of(f, n);
  CHECK(!tx_allowed(w.data(), w.size()));
  // Two frames back to back, trailing junk, truncated frame.
  n = proto::build_devtype_request(f, sizeof(f));
  Bytes one = wire_of(f, n);
  Bytes two = one;
  two.insert(two.end(), one.begin(), one.end());
  CHECK(!tx_allowed(two.data(), two.size()));
  Bytes junk = one;
  junk.push_back(0x00);
  CHECK(!tx_allowed(junk.data(), junk.size()));
  CHECK(!tx_allowed(one.data(), one.size() - 1));
  CHECK(!tx_allowed(nullptr, 0));
}

static void test_rate_limit_and_budget() {
  FakeBus bus;
  Engine e(&bus);
  uint32_t now = 0;
  uint8_t f[64];
  const size_t n = proto::build_devtype_request(f, sizeof(f));
  const Bytes w = wire_of(f, n);
  CHECK(e.send_raw_for_test(now, w.data(), w.size()));
  CHECK(!e.send_raw_for_test(now + 100, w.data(), w.size()));  // < 300 ms spacing
  CHECK(e.send_raw_for_test(now + 300, w.data(), w.size()));
  uint32_t t = now + 300;
  for (int i = 0; i < 30; i++) {
    t += 300;
    e.send_raw_for_test(t, w.data(), w.size());
  }
  CHECK(e.frames_sent() == Engine::MAX_FRAMES);
  CHECK(bus.writes.size() == Engine::MAX_FRAMES);
  // A refused frame never reaches the bus.
  uint8_t hb[64];
  const size_t hn = proto::build_heartbeat(0x01, 0x01, true, hb, sizeof(hb));
  const Bytes hw = wire_of(hb, hn);
  FakeBus bus2;
  Engine e2(&bus2);
  CHECK(!e2.send_raw_for_test(0, hw.data(), hw.size()));
  CHECK(bus2.writes.empty());
  CHECK(e2.refused_frames() == 1);

  // Spacing during a real probe: every pair of writes is >= 300 ms apart.
  FakeBus bus3;
  bus3.ac = FakeBus::Ac::DEVTYPE;
  Engine e3(&bus3);
  struct TimedBus : public Bus {
    FakeBus *inner;
    std::vector<uint32_t> times;
    explicit TimedBus(FakeBus *b) : inner(b) {}
    void configure(const UartProfile &p) override { this->inner->configure(p); }
    size_t read(uint8_t *buf, size_t cap) override { return this->inner->read(buf, cap); }
    void write(const uint8_t *d, size_t l) override {
      this->times.push_back(this->inner->now);
      this->inner->write(d, l);
    }
  } tb(&bus3);
  Engine e4(&tb);
  uint32_t t4 = 0;
  e4.start(t4, false);
  for (int i = 0; i < 20000 && e4.busy(); i++) {
    t4 += 1;
    bus3.now = t4;
    e4.tick(t4);
  }
  bool spaced = tb.times.size() >= 2;
  for (size_t i = 1; i < tb.times.size(); i++)
    spaced = spaced && tb.times[i] - tb.times[i - 1] >= Engine::MIN_TX_SPACING_MS;
  CHECK(spaced);
  CHECK(tb.times.size() >= 1 && tb.times[0] >= Engine::P1_LISTEN_MS);
  (void) e3;
}

// ---- T-U-3: capture, hex layout and report URL -------------------------------------------------
static void test_capture_ring() {
  Capture c;
  uint8_t d[600];
  for (size_t i = 0; i < sizeof(d); i++)
    d[i] = static_cast<uint8_t>(i);
  c.add(0, ChunkDir::RX, 0, d, 10, 0);
  c.add(5, ChunkDir::RX, 0, d + 10, 10, 20);  // within the gap: merged into one chunk
  ChunkView v{};
  CHECK(c.get(0, &v));
  CHECK(v.len == 20 && v.data[19] == 19);
  CHECK(!c.get(1, &v));
  c.add(100, ChunkDir::TX, 0, d, 20, 0);
  CHECK(c.get(1, &v) && v.dir == ChunkDir::TX && v.t_ms == 100);
  // Fill past 4 KiB: the oldest chunks are dropped, chunks are capped at 512 bytes.
  for (uint32_t i = 0; i < 20; i++)
    c.add(1000 + i * 100, ChunkDir::RX, 1, d, 600, 0);
  CHECK(c.dropped_chunks() > 0);
  size_t total = 0, count = 0;
  for (size_t i = 0; c.get(i, &v); i++) {
    CHECK(v.len <= 512);
    total += v.len + 8;
    count++;
  }
  CHECK(total <= 4096);
  CHECK(count >= 7);
  c.get(count - 1, &v);
  CHECK(v.t_ms == 1000 + 19 * 100);
}

static void test_hex_and_unstuff() {
  const uint8_t b[] = {0x00, 0x0A, 0xF4, 0xFF};
  CHECK(hex_spaced(b, 4) == "00 0A F4 FF");
  // A status request with type F4 F4 is stuffed on the wire; unstuff restores the decoded frame
  // so the decoded offsets (class at 13) line up.
  uint8_t f[64];
  const size_t n = proto::build_status_request(0xF4, 0xF4, 0x00, f, sizeof(f));
  const Bytes w = wire_of(f, n);
  CHECK(w.size() > n);
  uint8_t dec[64];
  const size_t dn = unstuff(w.data(), w.size(), dec, sizeof(dec));
  CHECK(dn == n);
  CHECK(dn == n && std::memcmp(dec, f, n) == 0);
  CHECK(dec[13] == 0x66);
  CHECK(tx_allowed(w.data(), w.size()));

  FakeBus bus;
  bus.ac = FakeBus::Ac::DEVTYPE;
  bus.echo = true;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  const std::string txt = e.capture_text("0.3.0", "m5stickc-plus2");
  CHECK(contains(txt, "outcome=detected"));
  CHECK(contains(txt, "TX   (20 bytes)"));
  CHECK(contains(txt, "  wire 0000  F4 F5 00 40 0B 00 00 00 00 FE 01 00 00 0A 04 00"));
  CHECK(contains(txt, "checksum ok | dir 01]"));
  CHECK(contains(txt, "ECHO"));
  const std::string cj = e.capture_json();
  CHECK(contains(cj, "\"format\":1"));
  CHECK(contains(cj, "\"dir\":\"tx\""));
  CHECK(contains(cj, "\"hex\":\"F4F500400B"));
  CHECK(contains(cj, "\"b64\":\"9PUAQAsA"));
}

static void test_report_url() {
  CHECK(url_encode("a b&c\n") == "a%20b%26c%0A");
  CHECK(json_escape("q\"\\\n") == "q\\\"\\\\\\n");
  FakeBus bus;
  bus.ac = FakeBus::Ac::GARBAGE;
  bus.echo = true;
  Engine e(&bus);
  uint32_t now = 0;
  e.start(now, false);
  run_until_done(e, bus, now);
  const std::string url = e.report_url("0.3.0-beta.1", "m5stickc-plus2");
  CHECK(url.rfind("https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml", 0) == 0);
  CHECK(url.size() <= 6000);
  CHECK(contains(url, "&title=%5BCompatibility%5D%20setup%20probe%3A%20unknown%20on%20m5stickc-plus2"));
  CHECK(contains(url, "&result=Does%20not%20work"));
  CHECK(contains(url, "&firmware=0.3.0-beta.1"));
  CHECK(contains(url, "ab_swap_likely"));
  CHECK(contains(url, "%60%60%60"));  // fenced hex block
  // A tight cap truncates the hex block but keeps a valid URL under the cap.
  const std::string small = e.report_url("0.3.0-beta.1", "m5stickc-plus2", 1500);
  CHECK(small.size() <= 1500);
  CHECK(contains(small, "%60%60%60%0A"));
  // Detected: no "Does not work" prefill.
  FakeBus b2;
  b2.ac = FakeBus::Ac::DEVTYPE;
  Engine e2(&b2);
  uint32_t n2 = 0;
  e2.start(n2, false);
  run_until_done(e2, b2, n2);
  const std::string u2 = e2.report_url("fw", "atom-lite-tail485");
  CHECK(!contains(u2, "&result="));
  CHECK(contains(u2, "detected%20on%20atom-lite-tail485"));
}

static void test_state_json() {
  FakeBus bus;
  Engine e(&bus);
  CHECK(contains(e.state_json(), "\"state\":\"idle\""));
  CHECK(contains(e.state_json(), "\"outcome\":\"none\""));
  uint32_t now = 0;
  const uint32_t seq0 = e.seq();
  e.start(now, false);
  CHECK(e.seq() > seq0);
  const std::string s = e.state_json();
  CHECK(contains(s, "\"state\":\"listen\""));
  CHECK(contains(s, "\"profile\":\"9600-8N1\""));
}

int main() {
  test_detect_devtype();
  test_detect_status_only();
  test_silence();
  test_echo_only();
  test_garbage_ab_swap();
  test_foreign_traffic();
  test_passive_only();
  test_validate_failed_and_timeout();
  test_allow_list();
  test_rate_limit_and_budget();
  test_capture_ring();
  test_hex_and_unstuff();
  test_report_url();
  test_state_json();
  std::printf("%d passed, %d failed\n", g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
