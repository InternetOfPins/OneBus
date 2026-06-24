/**
 * @file oneWire.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief 1-Wire bus HAPI component — bit-banged, parameterized on chip and pin.
 */

#pragma once
#include <hapi/hapi.h>

namespace oneBus {

  /// @brief 1-Wire bus protocol component; reset/readBit/writeByte/readByte
  template<typename Chip, typename Pin>
  struct OneWire {
    struct Part {
      static bool reset();
      static void writeBit(bool bit);
      static bool readBit();
      static void writeByte(uint8_t data);
      static uint8_t readByte();
    };
  };

} // oneBus
