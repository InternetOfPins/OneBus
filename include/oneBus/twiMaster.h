/**
 * @file twiMaster.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief TwiMaster duck-type concept — C++17 SFINAE trait.
 *
 * Any type T satisfying is_twi_master<T> provides a complete I2C master
 * interface usable by OneIO driver templates (AHT, PCA9685, AT24C, SSD1306, …):
 *
 *   static void    begin()
 *   static bool    begin_write(uint8_t addr)        — START + SLA+W; false only when the address is known
 *                                                     not to have answered (a buffered master returns true)
 *   static void    write_byte(uint8_t b)
 *   static bool    end_write()                      — STOP; true iff the whole transaction was acknowledged
 *   static uint8_t request_from(uint8_t addr, uint8_t n) — START + SLA+R; 0 when nothing answered
 *   static uint8_t read_byte()
 *
 * begin_write / end_write may still return void (older masters); probe() below copes with both.
 * A master may also provide cause() -> TwiCause and its own probe(addr, ProbeKind).
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
#include <oneBus/busAPI.h>

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
  // Also usable standalone as a null adapter: no bus, so nothing answers (reads return 0).
  struct TwiMasterTerm {
    TwiMasterTerm() = delete;
    static void    begin()                           {}
    static bool    begin_write(uint8_t)              { return false; }
    static void    write_byte(uint8_t)               {}
    static bool    end_write()                       { return false; }
    static uint8_t request_from(uint8_t, uint8_t)    { return 0; }
    [[nodiscard]] static uint8_t read_byte()                       { return 0; }
    static TwiCause cause()                          { return TwiCause::Nack; }
  };

  static_assert(is_twi_master<TwiMasterTerm>::value,
    "TwiMasterTerm must satisfy is_twi_master");

  // ── presence probing ────────────────────────────────────────────────────────
  // A probe touches no register. Write-probe: SLA+W, STOP. Read-probe: SLA+R, one byte read with NACK, STOP.
  enum class ProbeKind : uint8_t { Write, Read };

  // i2cdetect's default: read-probe where a zero-length write can disturb some EEPROMs (0x30-0x37, 0x50-0x5F).
  constexpr ProbeKind probeKindFor(uint8_t addr) {
    return ((addr >= 0x30 && addr <= 0x37) || (addr >= 0x50 && addr <= 0x5F)) ? ProbeKind::Read : ProbeKind::Write;
  }

  template<typename T, typename = void> struct has_native_probe : std::false_type {};
  template<typename T> struct has_native_probe<T, std::void_t<decltype(T::probe(std::declval<uint8_t>(), ProbeKind::Write))>>
    : std::true_type {};

  template<typename T, typename = void> struct has_cause : std::false_type {};
  template<typename T> struct has_cause<T, std::void_t<decltype(T::cause())>> : std::true_type {};

  /// the probe built from the concept alone
  template<typename Twi>
  bool probeWith(uint8_t addr, ProbeKind kind) {
    if (kind == ProbeKind::Read) {
      if (Twi::request_from(addr, 1) == 0) return false;
      (void)Twi::read_byte();
      return true;
    }
    (void)Twi::begin_write(addr);
    if constexpr (std::is_same<decltype(Twi::end_write()), void>::value) { Twi::end_write(); return true; }
    else return Twi::end_write();
  }

  /// true iff something acknowledged `addr`: the master's own probe if it has one, else probeWith
  template<typename Twi>
  bool probe(uint8_t addr, ProbeKind kind) {
    if constexpr (has_native_probe<Twi>::value) return Twi::probe(addr, kind);
    else return probeWith<Twi>(addr, kind);
  }
  template<typename Twi> bool probe(uint8_t addr) { return probe<Twi>(addr, probeKindFor(addr)); }

  /// why the master's last operation failed; None when it does not say
  template<typename Twi> TwiCause causeOf() {
    if constexpr (has_cause<Twi>::value) return Twi::cause();
    else return TwiCause::None;
  }

  /// how many addresses answered, and the bus fault that ended the scan early (None: every address was tried)
  struct ScanResult { uint8_t found; TwiCause stopped; };

  /// probes 0x08..0x77 (the 7-bit addresses a device may use) and calls found(addr) for each that answers. A Timeout,
  /// BusError or ArbLost is the bus's, not the address's: the scan stops there instead of repeating it 111 more times.
  template<typename Twi, typename Fn>
  ScanResult scan(Fn&& found) {
    uint8_t n = 0;
    for (uint8_t a = 0x08; a <= 0x77; ++a) {
      if (probe<Twi>(a)) { found(a); ++n; }
      else if (const TwiCause c = causeOf<Twi>(); isBusFault(c)) return {n, c};
    }
    return {n, TwiCause::None};
  }

} // oneBus
