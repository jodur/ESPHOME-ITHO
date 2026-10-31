#include "itho_protocol.h"
#include <cstring>

namespace esphome {
namespace itho_fan {

// Command bytes (data bytes 5-10), from ITHO-Lib IthoCC1101.h
static const uint8_t CMD_HIGH[] = {34, 241, 3, 0, 4, 4};
static const uint8_t CMD_RV_HIGH[] = {49, 224, 4, 0, 0, 200};
static const uint8_t CMD_MEDIUM[] = {34, 241, 3, 0, 3, 4};
static const uint8_t CMD_RV_MEDIUM[] = {34, 241, 3, 0, 3, 7};
static const uint8_t CMD_LOW[] = {34, 241, 3, 0, 2, 4};
static const uint8_t CMD_RV_LOW[] = {49, 224, 4, 0, 0, 1};
static const uint8_t CMD_RV_AUTO[] = {34, 241, 3, 0, 5, 7};
static const uint8_t CMD_TIMER1[] = {34, 243, 3, 0, 0, 10};  // 10 minutes full speed
static const uint8_t CMD_TIMER2[] = {34, 243, 3, 0, 0, 20};  // 20 minutes full speed
static const uint8_t CMD_TIMER3[] = {34, 243, 3, 0, 0, 30};  // 30 minutes full speed
static const uint8_t CMD_JOIN[] = {31, 201, 12, 0, 34, 241};
static const uint8_t CMD_JOIN2[] = {31, 201, 12, 99, 34, 248};  // RFT AUTO CO2 remote
static const uint8_t CMD_RV_JOIN[] = {31, 201, 24, 0, 49, 224};  // RFT-RV
static const uint8_t CMD_LEAVE[] = {31, 201, 6, 0, 31, 201};

bool itho_decode(const std::vector<uint8_t> &packet, IthoMessage &msg) {
  if (packet.size() != ITHO_PACKET_LENGTH || memcmp(packet.data(), ITHO_HEADER, sizeof(ITHO_HEADER)) != 0)
    return false;

  msg = IthoMessage();
  // Every 10 input bits contain 4 data bits on the even positions (0,2,4,6),
  // the inverted data bits on the odd positions (1,3,5,7) and a "10" separator.
  uint8_t out_i = 0, out_j = 4, chk_i = 0, chk_j = 4, bitcnt = 0;
  for (size_t i = sizeof(ITHO_HEADER); i < packet.size() && out_i < 32 && chk_i < 32; i++) {
    for (int j = 7; j >= 0; j--) {
      uint8_t bit = (packet[i] >> j) & 0x01;
      if (bitcnt < 8 && (bitcnt % 2) == 0) {
        msg.data[out_i] |= bit << out_j;
        if (++out_j > 7)
          out_j = 0;
        if (out_j == 4 && ++out_i >= 32)
          break;
      } else if (bitcnt < 8) {
        msg.data_chk[chk_i] |= bit << chk_j;
        if (++chk_j > 7)
          chk_j = 0;
        if (chk_j == 4) {
          msg.data_chk[chk_i] = ~msg.data_chk[chk_i];
          if (++chk_i >= 32)
            break;
        }
      }
      if (++bitcnt > 9)
        bitcnt = 0;
    }
  }
  return true;
}

// Compare the last 2 command bytes (IthoCC1101::checkIthoCommand)
static bool check_command(const IthoMessage &msg, const uint8_t cmd[6]) {
  // RFT-RV (24) and CO2 (28) remotes have the command bytes 2 positions further
  uint8_t offset = (msg.device_type() == 28 || msg.device_type() == 24) ? 2 : 0;
  for (int i = 4; i < 6; i++) {
    uint8_t pos = i + 5 + offset;
    if (msg.data[pos] != cmd[i] && msg.data_chk[pos] != cmd[i])
      return false;
  }
  return true;
}

IthoCommand itho_parse_command(const IthoMessage &msg) {
  // Same precedence as IthoCC1101::parseMessageCommand (last match wins)
  IthoCommand cmd = ITHO_UNKNOWN;
  if (check_command(msg, CMD_HIGH) || check_command(msg, CMD_RV_HIGH))
    cmd = ITHO_HIGH;
  if (check_command(msg, CMD_MEDIUM) || check_command(msg, CMD_RV_MEDIUM))
    cmd = ITHO_MEDIUM;
  if (check_command(msg, CMD_LOW) || check_command(msg, CMD_RV_LOW))
    cmd = ITHO_LOW;
  if (check_command(msg, CMD_RV_AUTO))
    cmd = ITHO_STANDBY;
  if (check_command(msg, CMD_TIMER1))
    cmd = ITHO_TIMER1;
  if (check_command(msg, CMD_TIMER2))
    cmd = ITHO_TIMER2;
  if (check_command(msg, CMD_TIMER3))
    cmd = ITHO_TIMER3;
  if (check_command(msg, CMD_JOIN) || check_command(msg, CMD_JOIN2) || check_command(msg, CMD_RV_JOIN))
    cmd = ITHO_JOIN;
  if (check_command(msg, CMD_LEAVE))
    cmd = ITHO_LEAVE;
  return cmd;
}

static const uint8_t *command_bytes(IthoCommand command) {
  switch (command) {
    case ITHO_HIGH:
    case ITHO_FULL:
      return CMD_HIGH;
    case ITHO_MEDIUM:
      return CMD_MEDIUM;
    case ITHO_TIMER1:
      return CMD_TIMER1;
    case ITHO_TIMER2:
      return CMD_TIMER2;
    case ITHO_TIMER3:
      return CMD_TIMER3;
    case ITHO_JOIN:
      return CMD_JOIN;
    case ITHO_LEAVE:
      return CMD_LEAVE;
    case ITHO_LOW:
    default:
      return CMD_LOW;
  }
}

// Manchester-like encoding (IthoCC1101::messageEncode): every data byte becomes
// 20 bits, low nibble first, each bit followed by its inverse and a "10"
// separator after every 4 data bits.
static void encode(const std::vector<uint8_t> &data, uint8_t end_byte, std::vector<uint8_t> &out) {
  out.assign(ITHO_PACKET_LENGTH, 0);
  memcpy(out.data(), ITHO_HEADER, sizeof(ITHO_HEADER));

  uint8_t out_byte = sizeof(ITHO_HEADER), out_bit = 0, pattern = 0, sel = 4, shift = 7;
  for (uint8_t b : data) {
    for (uint8_t n = 0; n < 8; n++) {
      if (out_bit == 8) {
        out_byte++;
        out_bit = 0;
      }
      if (pattern == 8) {  // insert "10"
        pattern = 0;
        out[out_byte] |= 1 << shift;
        shift--;
        out_bit++;
        if (shift == 0)
          shift = 8;
        shift--;
        out_bit++;
      }
      if (out_bit == 8) {
        out_byte++;
        out_bit = 0;
      }
      uint8_t bit = (b >> sel) & 0x01;
      if (++sel == 8)
        sel = 0;
      out[out_byte] |= bit << shift;  // data bit
      shift--;
      out_bit++;
      pattern++;
      out[out_byte] |= (bit ^ 1) << shift;  // inverted data bit
      if (shift == 0)
        shift = 8;
      shift--;
      out_bit++;
      pattern++;
    }
  }
  // closing "10" pattern to fill the last byte
  for (uint8_t i = out_bit; i < 8; i += 2) {
    out[out_byte] |= 1 << shift;
    shift--;
    if (shift == 0)
      shift = 8;
    shift--;
  }
  out_byte++;
  out[out_byte++] = end_byte;
  while (out_byte < ITHO_PACKET_LENGTH)
    out[out_byte++] = 0xAA;
}

std::vector<uint8_t> itho_build_packet(IthoCommand command, const uint8_t device_id[3], uint8_t counter) {
  std::vector<uint8_t> data = {ITHO_DEVICE_TYPE, device_id[0], device_id[1], device_id[2], counter};
  const uint8_t *cmd = command_bytes(command);
  data.insert(data.end(), cmd, cmd + 6);

  uint8_t end_byte = 0xAC;
  if (command == ITHO_JOIN) {
    data.insert(data.end(), device_id, device_id + 3);
    data.insert(data.end(), {0x01, 0x10, 0xE0});
    data.insert(data.end(), device_id, device_id + 3);
    end_byte = 0xCA;
  } else if (command == ITHO_LEAVE) {
    data.insert(data.end(), device_id, device_id + 3);
    end_byte = 0xCA;
  }

  // checksum: 0 - sum of all bytes
  uint8_t sum = 0;
  for (uint8_t v : data)
    sum += v;
  data.push_back(0 - sum);

  std::vector<uint8_t> packet;
  encode(data, end_byte, packet);
  return packet;
}

}  // namespace itho_fan
}  // namespace esphome
