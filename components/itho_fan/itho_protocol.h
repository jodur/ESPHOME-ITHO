#pragma once

// Itho RF protocol helpers (message encoding/decoding).
// Ported from ITHO-Lib IthoCC1101.cpp (Klusjesman, supersjimmie, arjenhiemstra).
// The radio itself is handled by the official ESPHome cc1101 component.

#include <cstdint>
#include <vector>

namespace esphome {
namespace itho_fan {

enum IthoCommand : uint8_t {
  ITHO_UNKNOWN = 0,
  ITHO_JOIN,
  ITHO_LEAVE,
  ITHO_STANDBY,
  ITHO_LOW,
  ITHO_MEDIUM,
  ITHO_HIGH,
  ITHO_FULL,
  ITHO_TIMER1,
  ITHO_TIMER2,
  ITHO_TIMER3,
};

// Size of a radio packet (cc1101 packet_length). The cc1101 adds preamble and
// sync word (0xAB 0xFE) on transmit and strips the sync word on receive.
static const uint8_t ITHO_PACKET_LENGTH = 63;

// Every Itho message starts with this header (directly after the sync word)
static const uint8_t ITHO_HEADER[5] = {0x00, 0xB3, 0x2A, 0xAB, 0x2A};

// Device type used when sending (standard Itho remote)
static const uint8_t ITHO_DEVICE_TYPE = 22;

struct IthoMessage {
  uint8_t data[32]{};      // decoded data bytes
  uint8_t data_chk[32]{};  // decoded from the inverted bits (used as alternative check)
  uint8_t device_type() const { return data[0]; }
  const uint8_t *device_id() const { return &data[1]; }
  uint8_t counter() const { return data[4]; }
};

// Decode a received radio packet. Returns false if it is not an Itho message.
bool itho_decode(const std::vector<uint8_t> &packet, IthoMessage &msg);

// Determine the command of a decoded message (remote control commands only)
IthoCommand itho_parse_command(const IthoMessage &msg);

// Build a radio packet for the given command, device ID and counter
std::vector<uint8_t> itho_build_packet(IthoCommand command, const uint8_t device_id[3], uint8_t counter);

}  // namespace itho_fan
}  // namespace esphome
