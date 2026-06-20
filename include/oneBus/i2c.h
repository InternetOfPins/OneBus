/**
 * @file i2c.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief I2C/TWI bus HAPI component — hardware master, parameterized on chip.
 */

#pragma once
#include <hapi/hapi.h>

namespace oneBus {

  template<typename Chip>
  struct I2c {
    struct Part {
      static void    init(uint32_t freq = 100000UL);
      static bool    start(uint8_t addr, bool write);
      static void    stop();
      static bool    write(uint8_t data);
      static uint8_t read(bool ack);
    };
  };

} // oneBus
