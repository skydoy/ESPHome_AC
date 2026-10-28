// Host unit tests for components/hisense_ac/hisense_protocol.{h,cpp}.
// Build and run:  make -C tests
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "../components/hisense_ac/hisense_protocol.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace esphome::hisense_ac::proto;

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

static bool same(const uint8_t *a, size_t an, const std::vector<uint8_t> &b) {
  return an == b.size() && std::memcmp(a, b.data(), an) == 0;
}

// Frames captured from a stock AEH-W41H1 module / AC, used as reference vectors.
static const std::vector<uint8_t> DEVTYPE_REQ = {0xF4, 0xF5, 0x00, 0x40, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFE,
                                                 0x01, 0x00, 0x00, 0x0A, 0x04, 0x00, 0x01, 0x58, 0xF4, 0xFB};
static const std::vector<uint8_t> VERSION_REQ = {0xF4, 0xF5, 0x00, 0x40, 0x0B, 0x00, 0x00, 0x01, 0x01, 0xFE,
                                                 0x01, 0x00, 0x00, 0x07, 0x01, 0x00, 0x01, 0x54, 0xF4, 0xFB};
static const std::vector<uint8_t> HEARTBEAT = {0xF4, 0xF5, 0x00, 0x40, 0x13, 0x00, 0x00, 0x01, 0x01, 0xFE,
                                               0x01, 0x00, 0x00, 0x1E, 0x00, 0x00, 0xB0, 0x80, 0x20, 0x00,
                                               0x00, 0x00, 0x40, 0x00, 0x03, 0x02, 0xF4, 0xFB};
static const std::vector<uint8_t> STATUS_REQ = {0xF4, 0xF5, 0x00, 0x40, 0x0C, 0x00, 0x00, 0x01, 0x01, 0xFE, 0x01,
                                                0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x01, 0xB3, 0xF4, 0xFB};
static const std::vector<uint8_t> DEVTYPE_REPLY = {0xF4, 0xF5, 0x01, 0x40, 0x0D, 0x01, 0x00, 0xFE,
                                                   0x01, 0x00, 0x00, 0x01, 0x00, 0x0A, 0x04, 0x01,
                                                   0x01, 0x01, 0x01, 0x60, 0xF4, 0xFB};

static void test_templates() {
  uint8_t f[64];
  size_t n = build_devtype_request(f, sizeof(f));
  CHECK(same(f, n, DEVTYPE_REQ));
  n = build_version_request(0x01, 0x01, f, sizeof(f));
  CHECK(same(f, n, VERSION_REQ));
  n = build_heartbeat(0x01, 0x01, true, f, sizeof(f));
  CHECK(same(f, n, HEARTBEAT));
  n = build_heartbeat(0x01, 0x01, false, f, sizeof(f));
  CHECK(n == HEARTBEAT.size() && f[16] == 0xF0 && frame_valid(f, n));
  n = build_status_request(0x01, 0x01, 0x00, f, sizeof(f));
  CHECK(same(f, n, STATUS_REQ));
  n = build_status_request(0x01, 0x01, SUB_PRODUCT_TYPE, f, sizeof(f));
  CHECK(n == 21 && f[14] == 0x40 && f[17] == 0x01 && f[18] == 0xF3 && frame_valid(f, n));
  CHECK(build_status_request(1, 1, 0, f, 10) == 0);
}

static void test_checksum_and_valid() {
  CHECK(checksum(STATUS_REQ.data(), STATUS_REQ.size()) == 0x01B3);
  CHECK(frame_valid(STATUS_REQ.data(), STATUS_REQ.size()));
  std::vector<uint8_t> bad = STATUS_REQ;
  bad[15] ^= 0x01;
  CHECK(!frame_valid(bad.data(), bad.size()));
  bad = STATUS_REQ;
  bad[4] = 0x0D;  // length mismatch
  CHECK(!frame_valid(bad.data(), bad.size()));
}

static void test_devtype_reply() {
  uint8_t hi = 0, lo = 0;
  CHECK(frame_valid(DEVTYPE_REPLY.data(), DEVTYPE_REPLY.size()));
  CHECK(parse_devtype(DEVTYPE_REPLY.data(), DEVTYPE_REPLY.size(), &hi, &lo));
  CHECK(hi == 0x01 && lo == 0x01);
}

static std::vector<uint8_t> wire_of(const uint8_t *f, size_t n) {
  uint8_t w[MAX_WIRE];
  size_t wn = stuff(f, n, w, sizeof(w));
  return std::vector<uint8_t>(w, w + wn);
}

static bool assemble(FrameAssembler &a, const std::vector<uint8_t> &wire, std::vector<uint8_t> *got) {
  bool any = false;
  for (uint8_t b : wire) {
    if (a.feed(b)) {
      any = true;
      if (got)
        got->assign(a.frame(), a.frame() + a.size());
    }
  }
  return any;
}

static void test_stuffing_roundtrip() {
  // Find a command whose checksum contains 0xF4 so the escape path is exercised.
  bool found = false;
  for (int sp = 16; sp <= 32 && !found; sp++) {
    for (int fan = 0; fan <= 5 && !found; fan++) {
     for (int mode = 0; mode <= 4 && !found; mode++) {
      Command c;
      c.has_power = c.power = true;
      c.has_mode = true;
      c.mode = static_cast<Mode>(mode);
      c.has_setpoint = true;
      c.setpoint = static_cast<uint8_t>(sp);
      c.has_vswing = true;
      c.has_fan = true;
      c.fan = static_cast<Fan>(fan);
      uint8_t f[COMMAND_LEN];
      build_command(c, 0x01, 0x01, f);
      if (f[46] == 0xF4 || f[47] == 0xF4) {
        found = true;
        std::vector<uint8_t> w = wire_of(f, COMMAND_LEN);
        CHECK(w.size() > COMMAND_LEN);
        FrameAssembler a;
        std::vector<uint8_t> got;
        CHECK(assemble(a, w, &got));
        CHECK(got.size() == COMMAND_LEN && std::memcmp(got.data(), f, COMMAND_LEN) == 0);
      }
     }
    }
  }
  CHECK(found);
  // Payload F4 byte too.
  uint8_t f[21];
  build_status_request(0xF4, 0x01, 0x00, f, sizeof(f));
  std::vector<uint8_t> w = wire_of(f, sizeof(f));
  CHECK(w.size() == sizeof(f) + 1);
  FrameAssembler a;
  std::vector<uint8_t> got;
  CHECK(assemble(a, w, &got));
  CHECK(got.size() == sizeof(f) && got[7] == 0xF4);
}

static void test_assembler_resync() {
  FrameAssembler a;
  std::vector<uint8_t> stream = {0x00, 0xF4, 0x13, 0xFF, 0xF4, 0xF5, 0x01, 0x40};  // truncated junk frame
  std::vector<uint8_t> w = wire_of(DEVTYPE_REPLY.data(), DEVTYPE_REPLY.size());
  stream.insert(stream.end(), w.begin(), w.end());
  stream.push_back(0x55);
  stream.insert(stream.end(), w.begin(), w.end());
  int frames = 0;
  for (uint8_t b : stream)
    if (a.feed(b))
      frames++;
  CHECK(frames == 2);

  // Corrupted checksum is counted and rejected.
  FrameAssembler b;
  std::vector<uint8_t> bad = DEVTYPE_REPLY;
  bad[19] = 0x61;
  CHECK(!assemble(b, bad, nullptr));
  CHECK(b.checksum_errors() == 1);
}

static std::vector<uint8_t> make_status(size_t len) {
  std::vector<uint8_t> f(len, 0);
  f[2] = 0x01;
  f[3] = 0x40;
  f[7] = 0xFE;
  f[8] = 0x01;
  f[13] = 0x66;
  f[14] = 0x00;
  f[16] = 0x0E;  // fan medium
  f[17] = 0x02;  // sleep profile 1
  f[18] = 0x2C;  // cool + running
  f[19] = 24;    // setpoint
  f[20] = 27;    // room
  f[21] = 12;    // indoor coil
  f[23] = 55;    // humidity
  f[35] = 0x80 | 0x04;  // vertical swing + eco
  f[36] = 0x04;  // quiet
  f[37] = 0x80;  // display on
  f[39] = 0x40;  // coil sensor fault
  f[41] = 60;
  f[42] = 58;
  f[44] = static_cast<uint8_t>(-3);
  f[45] = 40;
  if (len > 70) {
    f[64] = 0x08;
    f[66] = 0x80 | 0x02;
  }
  if (len >= 150) {
    f[50] = 228;
    f[55] = 15;
    f[56] = 0x00;
  }
  seal(f.data(), f.size());
  return f;
}

static void test_status_160() {
  std::vector<uint8_t> f = make_status(160);
  Status s;
  CHECK(parse_status(f.data(), f.size(), &s));
  CHECK(s.power && s.mode == Mode::COOL);
  CHECK(s.fan == Fan::MEDIUM && !s.fan_quiet_step);
  CHECK(s.sleep == 1);
  CHECK(s.setpoint == 24 && s.room_temp == 27 && s.indoor_coil_temp == 12);
  CHECK(s.has_humidity && s.humidity == 55);
  CHECK(s.vswing && !s.hswing && s.eco && !s.turbo && s.quiet && s.display);
  CHECK(s.has_outdoor && s.outdoor_valid && s.outdoor_temp == -3 && s.outdoor_coil_temp == 40);
  CHECK(s.compressor_hz == 58 && s.compressor_target_hz == 60);
  CHECK(s.faults.indoor == 0x40 && s.faults.outdoor == 0x08 && s.faults.protect == 0x02 && s.heat8);
  CHECK(s.has_electrical && s.voltage == 228);
  float p = power_watts(s);
  CHECK(p > 933.0f && p < 935.0f);
  std::string d = describe_faults(s.faults);
  CHECK(d == "Indoor coil sensor, Outdoor temperature sensor, High discharge temperature");
}

static void test_status_82_and_150() {
  std::vector<uint8_t> f = make_status(82);
  Status s;
  CHECK(parse_status(f.data(), f.size(), &s));
  CHECK(!s.has_electrical && s.has_outdoor);
  CHECK(power_watts(s) < 0);

  f = make_status(150);
  f[55] = 0x01;
  f[56] = 0x2C;
  seal(f.data(), f.size());
  CHECK(parse_status(f.data(), f.size(), &s));
  CHECK(power_watts(s) == 300.0f);

  // Fresh mains power: outdoor bytes read 0xEC.
  f = make_status(82);
  f[44] = 0xEC;
  f[45] = 0xEC;
  seal(f.data(), f.size());
  CHECK(parse_status(f.data(), f.size(), &s) && !s.outdoor_valid);

  // Modes and fan decoding.
  struct {
    uint8_t b18;
    bool on;
    Mode m;
  } modes[] = {{0x0C, true, Mode::FAN}, {0x1C, true, Mode::HEAT}, {0x30, false, Mode::DRY}, {0x5C, true, Mode::AUTO},
               {0x6C, true, Mode::AUTO}};
  for (auto &t : modes) {
    f = make_status(82);
    f[18] = t.b18;
    seal(f.data(), f.size());
    CHECK(parse_status(f.data(), f.size(), &s) && s.power == t.on && s.mode == t.m);
  }
  const uint8_t fans[] = {0x00, 0x01, 0x02, 0x0A, 0x0C, 0x0E, 0x10, 0x12};
  const Fan want[] = {Fan::AUTO, Fan::AUTO, Fan::AUTO, Fan::LOW, Fan::MEDIUM_LOW, Fan::MEDIUM, Fan::MEDIUM_HIGH, Fan::HIGH};
  for (size_t i = 0; i < sizeof(fans); i++) {
    f = make_status(82);
    f[16] = fans[i];
    seal(f.data(), f.size());
    CHECK(parse_status(f.data(), f.size(), &s) && s.fan == want[i]);
  }
  CHECK(s.fan_quiet_step == false);

  // Too short, wrong direction, or a ProductType reply: not a status frame.
  f = make_status(40);
  CHECK(!parse_status(f.data(), f.size(), &s));
  f = make_status(82);
  f[2] = 0x00;
  seal(f.data(), f.size());
  CHECK(!parse_status(f.data(), f.size(), &s));
}

static void test_capabilities() {
  std::vector<uint8_t> f = make_status(45);
  f[14] = 0x40;
  std::memset(&f[16], 0, 25);
  f[18] = 0x80;
  f[24] = 0x40;
  f[26] = 0x80;
  f[32] = 0x01;
  seal(f.data(), f.size());
  Capabilities c;
  CHECK(parse_capabilities(f.data(), f.size(), &c));
  CHECK(c.cool_heat && c.fan_mute && c.heat8 && c.humidity && !c.purify);
  Status s;
  CHECK(!parse_status(f.data(), f.size(), &s));
}

static void test_commands() {
  uint8_t f[COMMAND_LEN];
  Command c;
  c.has_power = c.power = true;
  c.has_mode = true;
  c.mode = Mode::COOL;
  c.has_setpoint = true;
  c.setpoint = 24;
  c.has_fan = true;
  c.fan = Fan::HIGH;
  build_command(c, 0x01, 0x01, f);
  CHECK(frame_valid(f, COMMAND_LEN));
  CHECK(f[4] == 0x29 && f[7] == 0x01 && f[8] == 0x01 && f[13] == 0x65);
  CHECK(f[16] == 0x13 && f[18] == 0x5C && f[19] == 49 && f[23] == 0x04 && f[31] == 0x01);
  CHECK(f[17] == 0 && f[32] == 0 && f[33] == 0 && f[35] == 0 && f[36] == 0 && f[37] == 0);

  const uint8_t mode_bytes[] = {0x1C, 0x3C, 0x5C, 0x7C, 0x9C};
  for (int m = 0; m <= 4; m++) {
    Command k;
    k.has_mode = true;
    k.mode = static_cast<Mode>(m);
    build_command(k, 1, 1, f);
    CHECK(f[18] == mode_bytes[m]);
  }
  const uint8_t fan_bytes[] = {0x01, 0x0B, 0x0D, 0x0F, 0x11, 0x13};
  for (int i = 0; i <= 5; i++) {
    Command k;
    k.has_fan = true;
    k.fan = static_cast<Fan>(i);
    build_command(k, 1, 1, f);
    CHECK(f[16] == fan_bytes[i]);
  }

  Command off;
  off.has_power = true;
  off.power = false;
  off.beep = false;
  build_command(off, 1, 1, f);
  CHECK(f[18] == 0x04 && f[23] == 0x00);

  Command sw;
  sw.has_vswing = sw.vswing = true;
  build_command(sw, 1, 1, f);
  CHECK(f[32] == 0xC0 && f[37] == 0x00);
  sw = Command();
  sw.has_hswing = true;
  sw.hswing = false;
  build_command(sw, 1, 1, f);
  CHECK(f[32] == 0x10 && f[37] == 0x14);

  Command sp;
  sp.has_eco = sp.eco = true;
  build_command(sp, 1, 1, f);
  CHECK(f[33] == 0x30);
  sp = Command();
  sp.has_turbo = sp.turbo = true;
  build_command(sp, 1, 1, f);
  CHECK(f[33] == 0x0C);
  sp = Command();
  sp.has_quiet = true;
  build_command(sp, 1, 1, f);
  CHECK(f[35] == 0x10);
  sp = Command();
  sp.has_display = sp.display = true;
  build_command(sp, 1, 1, f);
  CHECK(f[36] == 0xC0);
  sp = Command();
  sp.has_heat8 = sp.heat8 = true;
  build_command(sp, 1, 1, f);
  CHECK(f[37] == 0x03);
  sp = Command();
  sp.has_sleep = true;
  sp.sleep = 0;
  build_command(sp, 1, 1, f);
  CHECK(f[17] == 0x01);
  sp.sleep = 2;
  build_command(sp, 1, 1, f);
  CHECK(f[17] == 0x05);
}

int main() {
  test_templates();
  test_checksum_and_valid();
  test_devtype_reply();
  test_stuffing_roundtrip();
  test_assembler_resync();
  test_status_160();
  test_status_82_and_150();
  test_capabilities();
  test_commands();
  std::printf("%d passed, %d failed\n", g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
