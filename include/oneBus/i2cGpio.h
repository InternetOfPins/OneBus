#pragma once
#include <stdint.h>

namespace oneBus {

  // I2C GPIO expander virtual port (PCF8574, MCP23008, etc.)
  // TwiMaster : hardware TWI driver with send(addr, byte) interface.
  // Addr      : 7-bit I2C device address.
  // InitShadow: initial output state (all bits, written on first flush).
  //
  // Pin<N>::on/off/get implement the same interface as a physical OutPin —
  // they can be used wherever IOP expects a pin type.
  /// @brief I2C GPIO expander (PCF8574-style): shadow register + Pin<N> sub-struct for bit control
  template<typename TwiMaster, uint8_t Addr, uint8_t InitShadow = 0x00>
  struct I2cGpio {
    static inline uint8_t shadow = InitShadow;

    static void flush() { TwiMaster::send(Addr, shadow); }

    template<uint8_t N>
    struct Pin {
      static void on()         { I2cGpio::shadow |=  (1u << N); I2cGpio::flush(); }
      static void off()        { I2cGpio::shadow &= ~(1u << N); I2cGpio::flush(); }
      static void set(bool v)  { v ? on() : off(); }
      static bool get()        { return (I2cGpio::shadow >> N) & 1u; }
      static void begin()      { TwiMaster::begin(); }  // idempotent TWI init
      static void toggle()     { I2cGpio::shadow ^=  (1u << N); I2cGpio::flush(); }
    };
  };

} // oneBus
