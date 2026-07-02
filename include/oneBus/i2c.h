/**
 * @file i2c.h
 * @brief I2C/TWI protocol component + chip-family Twi<> aliases.
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>
 *   #include <oneBus/i2c.h>
 *   using Twi = chip::Twi<100000>;
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

// ── Chip-family Twi<> aliases ─────────────────────────────────────────────────
// chip::Twi<100000> resolves via the namespace alias set in the chip header.

#if defined(__AVR__)
  #include <chips/avr/avrTwi.h>
  namespace hw::avr {

    namespace mega {
      template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
      using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                              AvrTwiCore<CpuHz>>;
    }

    namespace mega2560 {
      template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
      using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                              AvrTwiCore<CpuHz>>;
    }

    namespace mega1284 {
      template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
      using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                              AvrTwiCore<CpuHz>>;
    }

  } // hw::avr

#elif defined(__arm__)
  #include <chips/stm32/stm32Twi.h>
  namespace hw::stm32 {

    namespace f1 {
      // Twi<> defined in stm32f103.h — included when the chip header is pulled in.
      // Direct composition available here for users without a chip catalog header.
      template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 36000000UL>
      using Twi_ = hapi::APIOf<oneBus::TwiAPI,
                               Stm32I2cCore<0x40005400u, Stm32F1_I2c1_PB6_PB7, ApbHz, SclHz>>;
    }

    namespace f4 {
      template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
      using Twi_ = hapi::APIOf<oneBus::TwiAPI,
                               Stm32I2cCore<0x40005400u, Stm32F4_I2c1_PB6_PB7, ApbHz, SclHz>>;
    }

    namespace f0 {
      // Stm32I2cV2Core, NOT Stm32I2cCore — F0's I2C is a different peripheral
      // design ("I2C_V2") from F1/F4's, see stm32Twi.h. ApbHz default 8MHz
      // matches f0::SysClk's HSI-reset default; only (8MHz,100kHz) and
      // (48MHz,100kHz) have verified TIMINGR constants — anything else is a
      // compile error by design, see stm32_i2c_v2_timing().
      template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 8000000UL>
      using Twi_ = hapi::APIOf<oneBus::TwiAPI,
                               Stm32I2cV2Core<0x40005400u, Stm32F0_I2c1_PA9_PA10, ApbHz, SclHz>>;
    }

  } // hw::stm32
#endif
