#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome::pm1006k {

class PM1006KComponent final : public PollingComponent, public uart::UARTDevice {
  SUB_SENSOR(pm_1_0)
  SUB_SENSOR(pm_2_5)
  SUB_SENSOR(pm_10_0)

 public:
  PM1006KComponent() = default;

  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

 protected:
  optional<bool> check_byte_() const;
  void parse_data_();
  uint8_t pm1006k_checksum_(const uint8_t *command_data, uint8_t length) const;
  uint16_t get_16_bit_uint_(uint8_t start_index) const {
    return encode_uint16(this->data_[start_index], this->data_[start_index + 1]);
  }

  uint8_t data_[16];
  uint8_t data_index_{0};
  uint32_t last_transmission_{0};
};

}  // namespace esphome::pm1006k
