/**
 * @file i2c.h
 * @brief I2C/TWI protocol component.
 *
 * Board-specific Twi<> aliases are defined in the chip headers:
 *   - chips/avr/avrTwi.h for AVR families
 *   - chips/stm32/stm32Twi.h for STM32 families
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>
 *   using Twi = chip::Twi<100000>;  // namespace alias from avrTwi.h
 *   Twi::begin();
 *   Twi::begin_write(0x27);
 *   Twi::write_byte(0xAB);
 *   Twi::end_write();
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>

namespace oneBus {

  /// @brief I2C master protocol component; begin_write/write_byte/end_write over the hardware core
  template<uint32_t Freq = 100000UL>
  struct I2cMaster {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() {
        Base::twi_init(Freq);
        Base::begin();
      }

      // ── Write streaming ──────────────────────────────────────────────────
      static void begin_write(uint8_t addr) {
        Base::twi_start();
        Base::twi_write(addr << 1);              // SLA+W
      }
      static void write_byte(uint8_t b) { Base::twi_write(b); }
      static void end_write()           { Base::twi_stop(); }

      static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
        begin_write(addr);
        while (len--) write_byte(*data++);
        end_write();
      }

      // ── Read streaming ───────────────────────────────────────────────────
      // request_from sends START + SLA+R and primes the byte counter.
      // read_byte ACKs all but the last byte, then sends STOP automatically.
      inline static uint8_t _rcount = 0;

      static uint8_t request_from(uint8_t addr, uint8_t n) {
        _rcount = n;
        Base::twi_start();
        Base::twi_write(uint8_t((addr << 1) | 1));  // SLA+R
        return n;
      }

      static uint8_t read_byte() {
        uint8_t b = Base::twi_read(_rcount > 1);    // ACK if more to come
        if (--_rcount == 0) Base::twi_stop();
        return b;
      }
    };
  };

} // oneBus

