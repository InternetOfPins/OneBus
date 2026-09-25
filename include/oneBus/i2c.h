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
#include <oneBus/twiMaster.h>

namespace oneBus {

  /// @brief I2C master protocol component; begin_write/write_byte/end_write over the hardware core.
  /// A core whose twi_start/twi_write return bool reports acknowledgement: a failed address or data byte makes
  /// end_write() false, releases the bus, and skips the rest of the transaction. A core returning void is
  /// treated as always acknowledging and costs nothing extra.
  template<uint32_t Freq = 100000UL>
  struct I2cMaster {
    template<typename O>
    struct Part : O {
      using Base = O;

    private:
      static constexpr bool kStarts = !std::is_same<decltype(Base::twi_start()), void>::value;
      static constexpr bool kAcks   = !std::is_same<decltype(Base::twi_write(uint8_t{})), void>::value;
      static constexpr bool kReport = kStarts || kAcks;

      inline static bool _open = false;   // this transaction holds the bus
      inline static bool _ok   = false;   // and everything so far was acknowledged

      static bool startBus() {
        if constexpr (kStarts) return Base::twi_start();
        else { Base::twi_start(); return true; }
      }
      static bool ackOf(uint8_t b) {
        if constexpr (kAcks) return Base::twi_write(b);
        else { Base::twi_write(b); return true; }
      }

    public:
      static void begin() {
        Base::twi_init(Freq);
        Base::begin();
      }

      // ── Write streaming ──────────────────────────────────────────────────
      // begin_write: false when the address did not answer. end_write: true iff the whole write was acknowledged.
      static bool begin_write(uint8_t addr) {
        if constexpr (kReport) {
          _open = startBus();
          _ok   = _open && ackOf(uint8_t(addr << 1));                 // SLA+W
          if (_open && !_ok) { Base::twi_stop(); _open = false; }     // NACK: a clean STOP, not a recovery
          return _ok;
        } else {
          Base::twi_start();
          Base::twi_write(addr << 1);                                 // SLA+W
          return true;
        }
      }
      static void write_byte(uint8_t b) {
        if constexpr (kAcks) { if (_ok) _ok = Base::twi_write(b); }
        else Base::twi_write(b);
      }
      static bool end_write() {
        if constexpr (kReport) {
          if (_open) { Base::twi_stop(); _open = false; }
          return _ok;
        } else {
          Base::twi_stop();
          return true;
        }
      }

      static bool send(uint8_t addr, const uint8_t* data, uint8_t len) {
        begin_write(addr);
        while (len--) write_byte(*data++);
        return end_write();
      }

      // ── Read streaming ───────────────────────────────────────────────────
      // request_from sends START + SLA+R and primes the byte counter; 0 when the address did not answer.
      // read_byte ACKs all but the last byte, then sends STOP automatically.
      inline static uint8_t _rcount = 0;

      [[nodiscard]] static uint8_t request_from(uint8_t addr, uint8_t n) {
        if constexpr (kReport) {
          _rcount = 0;
          if (!startBus()) return 0;
          if (!ackOf(uint8_t((addr << 1) | 1))) { Base::twi_stop(); return 0; }   // SLA+R
          _rcount = n;
          return n;
        } else {
          _rcount = n;
          Base::twi_start();
          Base::twi_write(uint8_t((addr << 1) | 1));                  // SLA+R
          return n;
        }
      }

      [[nodiscard]] static uint8_t read_byte() {
        if constexpr (kReport) { if (_rcount == 0) return 0xFF; }
        uint8_t b = Base::twi_read(_rcount > 1);                      // ACK if more to come
        if (--_rcount == 0) Base::twi_stop();
        return b;
      }

      // ── Presence and failure cause ───────────────────────────────────────
      static bool probe(uint8_t addr, ProbeKind kind) { return oneBus::probeWith<Part>(addr, kind); }
      static bool probe(uint8_t addr)                 { return oneBus::probe<Part>(addr); }

      static TwiCause cause() {
        if constexpr (has_twi_cause<Base>::value) return Base::twi_cause();
        else return TwiCause::None;
      }

    private:
      template<typename T, typename = void> struct has_twi_cause : std::false_type {};
      template<typename T> struct has_twi_cause<T, std::void_t<decltype(T::twi_cause())>> : std::true_type {};
    };
  };

} // oneBus

