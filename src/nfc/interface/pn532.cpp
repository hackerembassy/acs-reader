#include "pn532.h"
#include "pn532_constants.h"

#include "../../utils/debug.h"

#include <numeric>
#include <vector>
#include <cstdint>

std::vector<uint8_t> kPN532Ack{0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

// Function to calculate CRC16
std::vector<uint8_t> CalculateCRC16A(const std::vector<uint8_t>& data) {
  uint16_t w_crc = 0x6363;
  std::vector<uint8_t> result(2, 0);

  for (uint8_t byte : data) {
    byte = (byte ^ (w_crc & 0x00FF));
    byte = ((byte ^ (byte << 4)) & 0xFF);
    w_crc = ((w_crc >> 8) ^ (byte << 8) ^ (byte << 3) ^ (byte >> 4)) & 0xFFFF;
  }

  result[0] = static_cast<uint8_t>(w_crc & 0xFF);
  result[1] = static_cast<uint8_t>((w_crc >> 8) & 0xFF);

  return result;
}

// // Function to append CRC16 to data
// std::vector<uint8_t> with_crc16(std::vector<uint8_t> data) {
//   std::vector<uint8_t> result = CalculateCRC16A(data);

//   // Append CRC16 result to original data
//   data.insert(data.end(), result.begin(), result.end());

//   return data;
// }

bool PN532::Init() {
  pinMode(this->rst_pin_, OUTPUT);
  pinMode(this->irq_pin_, INPUT);

  digitalWrite(this->rst_pin_, HIGH);
  digitalWrite(this->rst_pin_, LOW);
  delay(400);
  digitalWrite(this->rst_pin_, HIGH);
  delay(10);

  DEBUG_PRINT("start i2c...\n");

  this->wire_.begin(this->sda_pin_, this->scl_pin_);
  this->wire_.setBufferSize(PN532_MAX_COMMAND_SIZE * 2u);

  uint32_t version;
  if (!this->GetFirmwareVersion(version)) {
    DEBUG_PRINT("ver get failed\n");
    return false;
  }

  if (version == 0) {
    DEBUG_PRINT("ver=0\n");
    return false;
  }

  if (!this->SetPassiveActivationRetries(0)) {
    DEBUG_PRINT("SPAR failed\n");
    return false;
  }

  if (!this->SAMConfigure()) {
    DEBUG_PRINT("SAMconf failed\n");
    return false;
  }

  DEBUG_PRINT("NFC: everything ok\n");

  return true;
}

bool PN532::FindTag(NFCTagInfo& info, uint32_t timeout) {
  std::vector<uint8_t> answer;
  if (!this->CommandExchange({PN532_COMMAND_INLISTPASSIVETARGET_REQUEST, 1, 0},
                             answer, timeout)) {
    return false;
  }
  if (answer[0] != PN532_COMMAND_INLISTPASSIVETARGET_RESPONSE) {
    DEBUG_PRINT("PN532 error: not ILPT resp\n");
    return false;
  }
  if (answer[1] != 1) {
   // DEBUG_PRINT("PN532 error: NbTg != 1\n");
    return false;
  }
    if (answer.size() < 7) {
   // DEBUG_PRINT("PN532 error: Size < 7\n");
    return false;
  }
  info.atqa = (answer[3] << 8ul) | answer[4];
  info.sak = answer[5];
  uint8_t uid_length = answer[6];
  if (answer.size() < (uid_length + 7)) {
    return false;
  }
  info.uid.clear();
  info.uid.insert(info.uid.begin(), answer.begin() + 7,
                  answer.begin() + 7 + uid_length);
  info.ats.clear();
  if (answer.size() == (uid_length + 7)) {
    return true;
  }
  uint8_t ats_length = answer[uid_length + 7];
  if (answer.size() < (uid_length + 7 + ats_length)) {
    return false;
  }
  info.ats.insert(info.ats.begin(), answer.begin() + 7 + uid_length,
                  answer.begin() + 7 + uid_length + ats_length);
  return true;
}

bool PN532::BroadcastECP(const std::vector<uint8_t>& data) {

  //DEBUG_PRINT("PN532 ecp: writing register\n");
  std::vector<uint8_t> answer_register;
  if (!this->CommandExchange(
          {PN532_COMMAND_WRITEREGISTER_REQUEST,
          static_cast<uint8_t>((0x633d >> 8) & 0xFF),
          static_cast<uint8_t>(0x633d & 0xFF),
          0x00},
          answer_register)) {
    return false;
  }

  if (answer_register.size() != 1) {
    DEBUG_PRINT("PN532 error: register response != 1\n");
    return false;
  }

  if (answer_register[0] != PN532_COMMAND_WRITEREGISTER_RESPONSE) {
    DEBUG_PRINT("PN532 error: not writereg resp\n");
    return false;
  }

  //DEBUG_PRINT("PN532 ecp: register write OK\n");

  std::vector<uint8_t> request{PN532_COMMAND_INCOMMUNICATETHRU_REQUEST};
  std::vector<uint8_t> answer;
  std::vector<uint8_t> crc_result = CalculateCRC16A(data);

  request.insert(request.end(), data.begin(), data.end());
  request.insert(request.end(), crc_result.begin(), crc_result.end());
  
  //DEBUG_PRINT("PN532 ecp: sending frame\n");
  this->CommandExchange(request, answer, 100);

  // if (!) {
  //   // we don't want errors as timeout is OK
  //   //return false;
  // }

  // if (answer.size() < 2) {
  //   DEBUG_PRINT("PN532 error: size < 2\n");
  //   return false;
  // }
  // if (answer[0] != PN532_COMMAND_INCOMMUNICATETHRU_RESPONSE) {
  //   DEBUG_PRINT("PN532 error: not IDEx resp\n");
  //   return false;
  // }

  //DEBUG_PRINT("PN532 ecp: frame sent\n");
  return true;
}

bool PN532::ApduExchange(const std::vector<uint8_t>& in_data,
                         std::vector<uint8_t>& out_data, uint32_t timeout) {
  std::vector<uint8_t> request{PN532_COMMAND_INDATAEXCHANGE_REQUEST, 1};
  request.insert(request.end(), in_data.begin(), in_data.end());
  std::vector<uint8_t> answer;
  if (!this->CommandExchange(request, answer, timeout)) {
    return false;
  }
  if (answer.size() < 2) {
    DEBUG_PRINT("PN532 error: size < 2\n");
    return false;
  }
  if (answer[0] != PN532_COMMAND_INDATAEXCHANGE_RESPONSE) {
    DEBUG_PRINT("PN532 error: not IDEx resp\n");
    return false;
  }
  if ((answer[1] & 0x3F) != 0) {
    DEBUG_PRINT("Failed to APDU: %02x\n", answer[1] & 0x3F);
    return false;
  }
  out_data.clear();
  out_data.insert(out_data.begin(), answer.begin() + 2, answer.end());
  return true;
}

bool PN532::SendCommandData(const std::vector<uint8_t>& data) {
  if (data.size() > PN532_MAX_COMMAND_SIZE - 1) {
    return false;
  }

  std::vector<uint8_t> command_data;
  command_data.push_back(PN532_PREAMBLE);
  command_data.push_back(PN532_PREAMBLE);
  command_data.push_back(PN532_STARTCODE2);
  if (data.size() <= 253) {
    uint8_t command_length = data.size() + 1;
    command_data.push_back(command_length);
    command_data.push_back(~command_length + 1);
  } else {
    command_data.push_back(0xFF);
    command_data.push_back(0xFF);
    uint16_t command_length = data.size() + 1;
    command_data.push_back(command_length >> 8);
    command_data.push_back(command_length & 0xFF);
    command_data.push_back(~(static_cast<uint8_t>((command_length >> 8) +
                                                  (command_length & 0xFF))) +
                           1);
  }

  command_data.push_back(PN532_HOSTTOPN532);
  command_data.insert(command_data.end(), data.begin(), data.end());

  uint8_t checksum =
      std::accumulate(data.begin(), data.end(), 0) + PN532_HOSTTOPN532;

  command_data.push_back(~checksum + 1);
  command_data.push_back(PN532_POSTAMBLE);

  this->wire_.beginTransmission(PN532_I2C_ADDRESS);
  if (this->wire_.write(command_data.data(), command_data.size()) !=
      command_data.size()) {
    this->wire_.endTransmission(true);
    return false;
  }
  this->wire_.endTransmission();

  return true;
}

bool PN532::ReadRdy() {
  return (this->wire_.read() & 1) == 1;
}

bool PN532::ReadAck(uint32_t timeout) {
  if (!this->WaitReady(timeout)) {
    return false;
  }

  this->wire_.requestFrom(PN532_I2C_ADDRESS, 1 + kPN532Ack.size(), true);

  if (!this->ReadRdy()) {
    return false;
  }

  std::vector<uint8_t> ack_buffer(kPN532Ack.size());
  if (this->wire_.readBytes(ack_buffer.data(), ack_buffer.size()) !=
      ack_buffer.size()) {
    return false;
  }
  if (ack_buffer != kPN532Ack) {
    return false;
  }
  return true;
}

bool PN532::ReadResponse(std::vector<uint8_t>& data, uint32_t timeout) {
  if (!this->WaitReady(timeout)) {
    return false;
  }

  this->wire_.requestFrom(
      PN532_I2C_ADDRESS,
      static_cast<size_t>(1 + 3 + 2 + 3 + 1 + PN532_MAX_COMMAND_SIZE + 2),
      true);

  if (!this->ReadRdy()) {
    return false;
  }

  std::vector<uint8_t> preamble_data(3);
  if (this->wire_.readBytes(preamble_data.data(), preamble_data.size()) !=
      preamble_data.size()) {
    return false;
  }

  if (preamble_data[0] != PN532_PREAMBLE ||
      preamble_data[1] != PN532_PREAMBLE ||
      preamble_data[2] != PN532_STARTCODE2) {
    return false;
  }

  std::vector<uint8_t> length_data(2);
  if (this->wire_.readBytes(length_data.data(), length_data.size()) !=
      length_data.size()) {
    return false;
  }

  bool is_extended = false;

  uint16_t length;
  if (length_data[0] == 0xFF && length_data[1] == 0xFF) {
    std::vector<uint8_t> extended_length_data(3);
    if (this->wire_.readBytes(extended_length_data.data(),
                              extended_length_data.size()) !=
        extended_length_data.size()) {
      return false;
    }

    if (extended_length_data[0] + extended_length_data[1] +
            extended_length_data[2] !=
        0) {
      return false;
    }

    length = (static_cast<uint16_t>(extended_length_data[0]) << 8) |
             extended_length_data[1];
    is_extended = true;
  } else {
    if (static_cast<uint8_t>(length_data[0] + length_data[1]) != 0) {
      return false;
    }
    length = length_data[0];
  }

  if (this->wire_.read() != PN532_PN532TOHOST) {
    return false;
  }

  std::vector<uint8_t> response_data{PN532_PN532TOHOST};
  response_data.reserve(length + 1);

  data.resize(length - 1);

  if (this->wire_.readBytes(data.data(), data.size()) != data.size()) {
    return false;
  }

  response_data.insert(response_data.end(), data.begin(), data.end());

  uint8_t checksum = this->wire_.read();
  response_data.push_back(checksum);

  if (static_cast<uint8_t>(std::accumulate(response_data.begin(),
                                           response_data.end(), 0)) != 0) {
    return false;
  }

  if (this->wire_.read() != PN532_POSTAMBLE) {
    return false;
  }

  return true;
}

bool PN532::CommandExchange(const std::vector<uint8_t>& in_data,
                            std::vector<uint8_t>& out_data, uint32_t timeout) {
  if (in_data.size() >= 255) {
    return false;
  }

  if (!this->SendCommandData(in_data)) {
    DEBUG_PRINT("PN532 error: sending cmd\n");
    return false;
  }

  if (!this->ReadAck(timeout)) {
    DEBUG_PRINT("PN532 error: no ack\n");
    return false;
  }

  if (!this->ReadResponse(out_data, timeout)) {
        DEBUG_PRINT("PN532 error: no rsp after ack\n");
    return false;
  }

  return true;
}

bool PN532::WaitReady(uint32_t timeout) {
  uint32_t elapsed = 0;
  while (elapsed < timeout) {
    if (digitalRead(this->irq_pin_) == 0) {
      return true;
    }
    delay(10);
    elapsed += 10;
  }
  DEBUG_PRINT("PN532 error: wait rdy timeout\n");
  return false;
}

bool PN532::GetFirmwareVersion(uint32_t& version) {
  std::vector<uint8_t> answer;
  if (!this->CommandExchange({PN532_COMMAND_GETFIRMWAREVERSION_REQUEST},
                             answer)) {
    return false;
  }

  if (answer.size() != 5) {
    return false;
  }

  if (answer[0] != PN532_COMMAND_GETFIRMWAREVERSION_RESPONSE) {
    return false;
  }

  version = (answer[0] << 24ul) | (answer[1] << 16ul) | (answer[2] << 8ul) |
            answer[3];
  return true;
}

bool PN532::SetPassiveActivationRetries(uint8_t max_retries) {
  std::vector<uint8_t> answer;
  if (!this->CommandExchange(
          {PN532_COMMAND_RFCONFIGURATION_REQUEST, 5, 0xFF, 0x01, max_retries},
          answer)) {
    return false;
  }

  if (answer.size() != 1) {
    return false;
  }

  if (answer[0] != PN532_COMMAND_RFCONFIGURATION_RESPONSE) {
    return false;
  }

  return true;
}

bool PN532::SAMConfigure() {
  std::vector<uint8_t> answer;
  if (!this->CommandExchange(
          {PN532_COMMAND_SAMCONFIGURATION_REQUEST, 1, 0x14, 1}, answer)) {
    return false;
  }

  if (answer.size() != 1) {
    return false;
  }

  if (answer[0] != PN532_COMMAND_SAMCONFIGURATION_RESPONSE) {
    return false;
  }

  return true;
}