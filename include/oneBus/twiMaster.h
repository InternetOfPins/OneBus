/**
 * @file twiMaster.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief TwiMaster duck-type concept — C++17 SFINAE trait.
 *
 * Any type T satisfying is_twi_master<T> provides a complete I2C master
 * interface usable by OneIO driver templates (AHT, PCA9685, AT24C, SSD1306, …):
 *
 *   static void    begin()
 *   static void    begin_write(uint8_t addr)        — START + SLA+W
 *   static void    write_byte(uint8_t b)
 *   static void    end_write()                      — STOP
 *   static uint8_t request_from(uint8_t addr, uint8_t n) — START + SLA+R
 *   static uint8_t read_byte()
 *
 * All methods are static (type-level bus, zero-overhead for embedded).
 *
 * Compatible implementations:
 *   chip::Twi<100000>          — AVR/STM32 hardware TWI  (OneBus I2cMaster + chip core)
 *   ArduinoWire<Wire>          — Arduino TwoWire adapter  (OneIO, #ifdef ARDUINO)
 *   hw::native::LinuxTwi<1>   — Linux /dev/i2c-1          (OneChip native)
 *   hw::native::VirtualTwi    — in-memory bus for tests   (OneChip native)
 *
 * Usage in a driver:
 *   template<typename Twi>
 *   struct MyDriver {
 *     static_assert(oneBus::is_twi_master<Twi>::value,
 *       "Twi must satisfy TwiMaster — see OneBus/twiMaster.h");
 *   };
 */

#pragma once
#ifdef __AVR__
  #include <hapi/platform/avr/avr_std.h>
  #include <stdint.h>
#else
  #include <type_traits>
  #include <cstdint>
#endif

namespace oneBus {

  template<typename T, typename = void>
  struct is_twi_master : std::false_type {};

  template<typename T>
  struct is_twi_master<T, std::void_t<
    decltype(T::begin()),
    decltype(T::begin_write(std::declval<uint8_t>())),
    decltype(T::write_byte(std::declval<uint8_t>())),
    decltype(T::end_write()),
    decltype(T::request_from(std::declval<uint8_t>(), std::declval<uint8_t>())),
    decltype(T::read_byte())
  >> : std::true_type {};

  // ── TwiMasterTerm — silent terminal ─────────────────────────────────────────
  // Parallel to onePin::InPin / oneBus::BusAPI.
  // Satisfies is_twi_master<> so hardware components can chain via Part<O>.
  // Also usable standalone as a null adapter (reads return 0).
  struct TwiMasterTerm {
    TwiMasterTerm() = delete;
    static void    begin()                           {}
    static void    begin_write(uint8_t)              {}
    static void    write_byte(uint8_t)               {}
    static void    end_write()                       {}
    static uint8_t request_from(uint8_t, uint8_t n) { return n; }
    static uint8_t read_byte()                       { return 0; }
  };

  static_assert(is_twi_master<TwiMasterTerm>::value,
    "TwiMasterTerm must satisfy is_twi_master");

} // oneBus
