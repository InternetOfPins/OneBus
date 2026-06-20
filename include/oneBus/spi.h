/**
 * @file spi.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief SPI bus HAPI component — hardware master, parameterized on chip.
 */

#pragma once
#include <hapi/hapi.h>

namespace oneBus {

  template<typename Chip>
  struct Spi {
    struct Part {
      static void init(uint8_t clockDiv = 4);
      static void select();
      static void deselect();
      static uint8_t transfer(uint8_t data);
      static void transferBuf(uint8_t* buf, uint16_t len);
    };
  };

} // oneBus
