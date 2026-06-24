/**
 * @file oneWire.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief 1-Wire bus: protocol component + platform cores.
 *
 * Two cores are provided:
 *
 *   ArduinoOneWireCore<PinN>  — Arduino GPIO API; works on AVR, ESP32, ESP8266.
 *                               Simple to use; timing relies on delayMicroseconds().
 *   AvrOneWireCore<Port, Bit> — direct DDR/PORT/PIN registers + _delay_us();
 *                               cycle-accurate, cli/sei guarded. AVR only.
 *                               Port: e.g. hw::avr::chip::PortC; Bit: 0-7.
 *
 * Assembled aliases:
 *
 *   // Any Arduino target (Arduino pin number)
 *   using MyBus = oneBus::OneWire<2>;
 *
 *   // AVR with cycle-accurate timing (requires OneChip port type)
 *   #include <chips/avr/avrPort.h>
 *   using MyBus = oneBus::AvrOneWire<hw::avr::chip::PortC, 2>;  // PC2 = A2
 *
 * Both yield the same protocol API: begin(), reset(), writeByte(), readByte(), skip().
 * DS18B20 requires an external 4.7 kΩ pull-up resistor on the data line.
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>
#include <stdint.h>

namespace oneBus {

  // 1-Wire bus terminal — primitives a hardware core must implement.
  struct OneWireAPI : BusAPI {
    OneWireAPI() = delete;
    static bool ow_reset()         = delete;  // 480μs reset; returns true if device present
    static void ow_write_bit(bool) = delete;
    static bool ow_read_bit()      = delete;
  };

  // Protocol layer — sits on top of any OneWireCore.
  /// @brief 1-Wire protocol component; writeByte/readByte/skip over ow_reset/ow_write_bit/ow_read_bit
  struct OneWireMaster {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin()  { Base::begin(); }
      static bool reset()  { return Base::ow_reset(); }

      static void writeByte(uint8_t b) {
        for (uint8_t i = 0; i < 8; ++i) { Base::ow_write_bit(b & 1u); b >>= 1; }
      }
      static uint8_t readByte() {
        uint8_t b = 0;
        for (uint8_t i = 0; i < 8; ++i)
          if (Base::ow_read_bit()) b |= uint8_t(1u << i);
        return b;
      }
      // Skip ROM — address the single device without reading its 64-bit ID
      static void skip() { writeByte(0xCC); }
    };
  };

} // oneBus

// ── Arduino bit-bang core ─────────────────────────────────────────────────────
// Works on any Arduino-compatible target: AVR, ESP32, ESP8266.
// Timing uses delayMicroseconds(); noInterrupts()/interrupts() for critical windows.
#ifdef ARDUINO
#include <Arduino.h>

namespace oneBus {

  /// @brief Arduino bit-bang 1-Wire core; HAPI component — ow_reset/ow_write_bit/ow_read_bit
  template<int PinN>
  struct ArduinoOneWireCore {
    template<typename O>
    struct Part : O {
      static void begin() { pinMode(PinN, INPUT_PULLUP); O::begin(); }

      static bool ow_reset() {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        digitalWrite(PinN, LOW);
        interrupts();
        delayMicroseconds(480);
        noInterrupts();
        pinMode(PinN, INPUT_PULLUP);
        delayMicroseconds(70);
        bool present = !digitalRead(PinN);
        interrupts();
        delayMicroseconds(410);
        return present;
      }

      static void ow_write_bit(bool bit) {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        if (bit) {
          digitalWrite(PinN, LOW);
          delayMicroseconds(6);
          pinMode(PinN, INPUT_PULLUP);
          interrupts();
          delayMicroseconds(64);
        } else {
          digitalWrite(PinN, LOW);
          interrupts();
          delayMicroseconds(60);
          noInterrupts();
          pinMode(PinN, INPUT_PULLUP);
          interrupts();
          delayMicroseconds(10);
        }
      }

      static bool ow_read_bit() {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        digitalWrite(PinN, LOW);
        delayMicroseconds(6);
        pinMode(PinN, INPUT_PULLUP);
        delayMicroseconds(9);
        bool bit = digitalRead(PinN);
        interrupts();
        delayMicroseconds(55);
        return bit;
      }
    };
  };

  // Ready-to-use assembled 1-Wire bus: oneBus::OneWire<2>  (Arduino pin 2)
  template<int PinN>
  using OneWire = hapi::APIOf<OneWireAPI, OneWireMaster, ArduinoOneWireCore<PinN>>;

} // oneBus

#endif // ARDUINO

// ── AVR native core ───────────────────────────────────────────────────────────
// Direct DDR/PORT/PIN register access + _delay_us() (cycle-accurate, no runtime
// overhead). cli()/sei() guard the critical windows in each bit slot.
// Port: AVRPort type from OneChip (e.g. hw::avr::chip::PortC).
// Bit: pin index within the port (0-7).
// Requires F_CPU to be defined (standard with avr-libc + Arduino framework).
#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

namespace oneBus {

  /// @brief AVR direct-register 1-Wire core; cli/sei + _delay_us(); cycle-accurate
  template<typename Port, uint8_t Bit>
  struct AvrOneWireCore {
    template<typename O>
    struct Part : O {
      static constexpr uint8_t kMask = uint8_t(1u << Bit);

      static volatile uint8_t& ddr()  { return *reinterpret_cast<volatile uint8_t*>(Port::ddrAddr); }
      static volatile uint8_t& port() { return *reinterpret_cast<volatile uint8_t*>(Port::portAddr); }
      static volatile uint8_t& pin()  { return *reinterpret_cast<volatile uint8_t*>(Port::pinAddr); }

      // Drive LOW: set DDR=1 (PORT is pre-cleared in begin() to 0)
      static void _low()    { ddr() |= kMask; }
      // Release: set DDR=0 (input), PORT=1 (internal pull-up on — supplements external 4.7kΩ)
      static void _hi_z()   { ddr() &= ~kMask; port() |= kMask; }
      // Read current line state
      static bool _sense()  { return (pin() & kMask) != 0; }

      static void begin() {
        port() &= ~kMask;   // PORT=0: when DDR switches to output, line immediately pulls LOW
        ddr()  &= ~kMask;   // start as input (external pull-up holds line HIGH)
        port() |=  kMask;   // enable internal pull-up (supplements external)
        O::begin();
      }

      // 1-Wire reset: 480 μs LOW, then sample presence pulse at 70 μs.
      // Interrupts enabled during long LOW pulse and recovery; disabled only
      // during the 70 μs sample window where a 1-2 μs glitch would be wrong.
      static bool ow_reset() {
        cli();
        _low();
        sei();
        _delay_us(480);
        cli();
        _hi_z();
        _delay_us(70);
        bool present = !_sense();
        sei();
        _delay_us(410);
        return present;
      }

      // Write-1 slot: LOW for 6 μs, then release; 64 μs recovery.
      // Write-0 slot: LOW for 60 μs, then release; 10 μs recovery.
      // Critical window (cli) covers only the tight pull-down edges.
      static void ow_write_bit(bool bit) {
        if (bit) {
          cli();
          _low();
          _delay_us(6);
          _hi_z();
          sei();
          _delay_us(64);
        } else {
          cli();
          _low();
          sei();
          _delay_us(60);
          cli();
          _hi_z();
          sei();
          _delay_us(10);
        }
      }

      // Read slot: LOW for 6 μs, release, sample at 9 μs, 55 μs recovery.
      // Interrupts disabled through sample point to prevent read jitter.
      static bool ow_read_bit() {
        cli();
        _low();
        _delay_us(6);
        _hi_z();
        _delay_us(9);
        bool bit = _sense();
        sei();
        _delay_us(55);
        return bit;
      }
    };
  };

  // Ready-to-use assembled AVR 1-Wire bus.
  // #include <chips/avr/avrPort.h> then:
  //   using MyBus = oneBus::AvrOneWire<hw::avr::chip::PortC, 2>;  // PC2
  template<typename Port, uint8_t Bit>
  using AvrOneWire = hapi::APIOf<OneWireAPI, OneWireMaster, AvrOneWireCore<Port, Bit>>;

} // oneBus

#endif // __AVR__
