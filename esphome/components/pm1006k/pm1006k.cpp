#include "pm1006k.h"
#include "esphome/core/log.h"

namespace esphome::pm1006k {

ESPHOME_LOG_TAG(TAG, "pm1006k");

static const uint8_t PM1006K_RESPONSE_HEADER[] = {0x16, 0x11, 0x0B};
static const uint8_t PM1006K_REQUEST[] = {0x11, 0x01, 0x02, 0xEC};

// Общая длина кадра PM1006K: header 3 + data 12 + checksum 1 = 16
static const uint8_t PM1006K_FRAME_LENGTH = 16;

void PM1006KComponent::setup() {
  // Отправляем команду сразу после включения, чтобы датчик не переключился в PWM
  ESP_LOGV(TAG, "sending initial measurement request to keep UART mode");
  this->write_array(PM1006K_REQUEST, sizeof(PM1006K_REQUEST));
}

void PM1006KComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PM1006K:");
  LOG_SENSOR("  ", "PM1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM10", this->pm_10_0_sensor_);
  LOG_UPDATE_INTERVAL(this);
}

void PM1006KComponent::update() {
  ESP_LOGV(TAG, "sending measurement request");
  this->write_array(PM1006K_REQUEST, sizeof(PM1006K_REQUEST));
}

void PM1006KComponent::loop() {
  this->read_available([this](uint8_t byte) {
    this->data_[this->data_index_] = byte;
    auto check = this->check_byte_();
    if (!check.has_value()) {
      // finished
      this->parse_data_();
      this->data_index_ = 0;
    } else if (!*check) {
      // wrong data
      ESP_LOGV(TAG, "Byte %i of received data frame is invalid.", this->data_index_);
      this->data_index_ = 0;
    } else {
      // next byte
      this->data_index_++;
    }
  });
}

uint8_t PM1006KComponent::pm1006k_checksum_(const uint8_t *command_data, uint8_t length) const {
  return sum_bytes(command_data, length);
}

optional<bool> PM1006KComponent::check_byte_() const {
  const uint8_t index = this->data_index_;
  const uint8_t byte = this->data_[index];

  // index 0..2 are the fixed header
  if (index < sizeof(PM1006K_RESPONSE_HEADER)) {
    return byte == PM1006K_RESPONSE_HEADER[index];
  }

  // index 3..14 are the data fields (DF1..DF12)
  if (index < (PM1006K_FRAME_LENGTH - 1)) {
    return true;
  }

  // checksum at index 15
  if (index == (PM1006K_FRAME_LENGTH - 1)) {
    uint8_t checksum = pm1006k_checksum_(this->data_, PM1006K_FRAME_LENGTH);
    if (checksum != 0) {
      ESP_LOGW(TAG, "PM1006K checksum is wrong: %02x, expected zero", checksum);
      return false;
    }
    return {};
  }

  return false;
}

void PM1006KComponent::parse_data_() {
  // Раскладка данных PM1006K:
  // data_[0..2]   = 0x16 0x11 0x0B
  // data_[3..4]   = DF1-DF2  -> PM1.0
  // data_[5..6]   = DF3-DF4  -> PM2.5
  // data_[7..8]   = DF5-DF6  -> PM10
  // data_[9..14]  = DF7-DF12 (не используются)
  // data_[15]     = checksum
  const uint16_t pm_1_0 = this->get_16_bit_uint_(3);
  const uint16_t pm_2_5 = this->get_16_bit_uint_(5);
  const uint16_t pm_10_0 = this->get_16_bit_uint_(7);

  ESP_LOGD(TAG, "Got PM1.0: %d, PM2.5: %d, PM10: %d µg/m³", pm_1_0, pm_2_5, pm_10_0);

  PUBLISH_STATE(this->pm_1_0_sensor_, pm_1_0);
  PUBLISH_STATE(this->pm_2_5_sensor_, pm_2_5);
  PUBLISH_STATE(this->pm_10_0_sensor_, pm_10_0);
}

}  // namespace esphome::pm1006k
